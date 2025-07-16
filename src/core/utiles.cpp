#include "utiles.hpp"
#include <sstream>
#include <vector>
#include <netinet/in.h> // For htonl function (allowed)

unsigned long parseIpAddress(const std::string &ip)
{
    // Handle special cases
    if (ip == "localhost")
    {
        return htonl(0x7F000001); // 127.0.0.1 in network byte order
    }

    if (ip == "0.0.0.0" || ip.empty())
    {
        return htonl(0x00000000); // 0.0.0.0 in network byte order
    }

    // Parse IPv4 address manually
    std::vector<int> octets;
    std::string octet;
    size_t start = 0;
    size_t pos = 0;

    // Split by dots manually (can't use getline with delimiter)
    while (pos <= ip.length())
    {
        if (pos == ip.length() || ip[pos] == '.')
        {
            if (pos > start)
            {
                octet = ip.substr(start, pos - start);

                if (octet.empty())
                {
                    return 0; // Invalid
                }

                // Convert string to number manually
                int num = 0;
                for (size_t i = 0; i < octet.length(); ++i)
                {
                    char c = octet[i];
                    if (c < '0' || c > '9')
                    {
                        return 0; // Invalid
                    }
                    num = num * 10 + (c - '0');
                }

                // Check if octet is in valid range (0-255)
                if (num < 0 || num > 255)
                {
                    return 0; // Invalid
                }

                octets.push_back(num);
            }
            start = pos + 1;
        }
        pos++;
    }

    // Must have exactly 4 octets
    if (octets.size() != 4)
    {
        return 0; // Invalid
    }

    // Convert to network byte order using only allowed functions
    unsigned long ip_addr = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
    return htonl(ip_addr);
}

bool isValidIpFormat(const std::string &ip)
{
    return parseIpAddress(ip) != 0 || ip == "0.0.0.0";
}

std::string resolveHostname(const std::string &hostname)
{
    if (hostname == "localhost")
    {
        return "127.0.0.1";
    }
    // For webserv, we typically only handle localhost and direct IP addresses
    // Return the hostname as-is if it's already an IP
    if (isValidIpFormat(hostname))
    {
        return hostname;
    }

    // Default fallback
    return "127.0.0.1";
}
