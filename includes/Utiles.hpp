#pragma once
#include <string>

unsigned long parseIpAddress(const std::string &ip);
std::string itoa(unsigned long value);
std::string parseClientAddress(const struct sockaddr_in &clientAddr);
bool isValidIpFormat(const std::string &ip);
std::string resolveHostname(const std::string &hostname);
void ft_memset(void *ptr, int value, size_t num);