#pragma once

#include "HttpServer.hpp"
#include "Parser.hpp"

extern HttpServer* g_server;

void signalHandler(int signal);

