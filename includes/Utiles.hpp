#pragma once
#include <string>

// Convert IP address string to network byte order (no external functions)
unsigned long parseIpAddress(const std::string &ip);
// Convert unsigned long to string representation
std::string itoa(unsigned long value);

// parse client address from sockaddr_in structure
std::string parseClientAddress(const struct sockaddr_in &clientAddr);

// Check if string is a valid IPv4 address format
bool isValidIpFormat(const std::string &ip);

// Convert hostname to IP address (for localhost, etc.)
std::string resolveHostname(const std::string &hostname);

// Memory set function to zero out memory
void ft_memset(void *ptr, int value, size_t num);