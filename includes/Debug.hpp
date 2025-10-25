#pragma once

#include "Config.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

void printRawConfig(const Config &config, int indent = 0);
void printParsedConfig(const Config &config);
void printConfigSummary(const Config &config);

void printServerStartup(const Config &config);
void printListenAddresses(const Config &config);

void logRequest(const HttpRequest& request);
void logResponse(const HttpResponse& response);

void validateConfigCompliance(const Config &config);
bool checkPortAvailability(int port);
void printValidationSummary(const Config &config);
