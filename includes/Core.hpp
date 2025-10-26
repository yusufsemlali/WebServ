#pragma once

#include <iostream>
#include "Parser.hpp"
#include "HttpServer.hpp"

// Global server pointer for signal handling
extern HttpServer* g_server;

// Signal handler function
void signalHandler(int signal);

