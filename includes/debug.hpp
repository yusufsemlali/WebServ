#pragma once

#include <iostream>

#include "Config.hpp"

// Configuration debugging functions
void printRawConfig(const Config &config, int indent = 0);
void printParsedConfig(const Config &config);
void printConfigSummary(const Config &config);

// Server debugging functions
void printServerStartup(const Config &config);
void printListenAddresses(const Config &config);

// HTTP debugging functions (for future use)
// void logRequest(const HttpRequest& request);
// void logResponse(const HttpResponse& response);

// Validation and compliance checking
void validateConfigCompliance(const Config &config);
bool checkPortAvailability(int port);
void printValidationSummary(const Config &config);
