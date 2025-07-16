#pragma once
#include <string>

// Convert IP address string to network byte order (no external functions)
unsigned long parseIpAddress(const std::string& ip);

// Check if string is a valid IPv4 address format
bool isValidIpFormat(const std::string& ip);

// Convert hostname to IP address (for localhost, etc.)
std::string resolveHostname(const std::string& hostname);
