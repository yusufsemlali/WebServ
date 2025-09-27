#pragma once

#include <map>
#include <string>

#include "ClientConnection.hpp"
#include "Config.hpp"
#include "EventLoop.hpp"
#include "RequestHandler.hpp"
#include "SocketManager.hpp"

class HttpServer
{
   public:
    HttpServer(Config &config);
    ~HttpServer();

    void run();
    void stop();

   private:
    Config config;
    SocketManager socketManager;
    EventLoop eventLoop;
    RequestHandler requestHandler;

    bool running;

    std::map<int, ClientConnection *> connections;
    std::map<int, ClientConnection *> cgiConnections;  // Map CGI FD to client connection

    bool initializeServers();

    void handleNewConnection(int serverFd);
    void handleClientRead(int clientFd);
    void handleClientWrite(int clientFd);
    void handleClientError(int clientFd);
    void handleCgiRead(int cgiFd);     // New: Handle CGI output ready
    void handleCgiError(int cgiFd);    // New: Handle CGI errors

    void closeConnection(int clientFd);
    bool isServerSocket(int fd) const;
    bool isCgiSocket(int fd) const;    // New: Check if FD is CGI socket

    void checkTimeouts();
};
