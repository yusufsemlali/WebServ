#pragma once
#include "Config.hpp"
#include "SocketManager.hpp"
#include "EventLoop.hpp"
#include "RequestHandler.hpp"

class HttpServer
{

public:
        HttpServer(Config &config);
        ~HttpServer();

        int start();
        void stop();

private:
        Config config;
        SocketManager socketManager;
        EventLoop eventLoop;
        RequestHandler requestHandler;
        
        bool running;
        
        void startServer(const Config::ServerConfig &server);
        bool initializeServers();
        void cleanup();

};
