#pragma once

#include <map>

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
    std::map<int, ClientConnection *> cgiConnections;

    bool initializeServers();

    void handleNewConnection(int serverFd);
    void handleClientRead(int clientFd);
    void handleClientWrite(int clientFd);
    void handleClientError(int clientFd);
    void handleCgiRead(int cgiFd);
    void handleCgiError(int cgiFd);

    void closeConnection(int clientFd);
    void checkCgiTimeouts();
    
    bool isServerSocket(int fd) const;
    bool isCgiSocket(int fd) const;

};
