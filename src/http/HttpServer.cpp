#include "HttpServer.hpp"
#include <iostream>

HttpServer::HttpServer(Config &config) : config(config), eventLoop(socketManager, config), requestHandler(config), running(false) {}

HttpServer::~HttpServer() { cleanup(); }

int HttpServer::start()
{
        // Initialize servers
        if (!initializeServers())
        {
                throw std::runtime_error("Failed to initialize servers");
        }

        // Initialize event loop
        if (!eventLoop.initialize())
        {
                throw std::runtime_error("Failed to initialize event loop");
        }

        std::cout << "HTTP Server started successfully" << std::endl;
        running = true;

        // Start main event loop
        eventLoop.run();

        return 0; // Success
}

void HttpServer::stop()
{
        running = false;
        eventLoop.stop();
}

void HttpServer::startServer(const Config::ServerConfig &server)
{
        // Create server sockets for each listen directive
        for (size_t i = 0; i < server.listenConfigs.size(); ++i)
        {
                const Config::ListenConfig &listenConfig = server.listenConfigs[i];
                if (!socketManager.createServerSocket(listenConfig))
                {
                        std::cerr << "Failed to create server socket for "
                                  << listenConfig.host << ":" << listenConfig.port << std::endl;
                }
                else
                {
                        std::cout << "Server listening on "
                                  << listenConfig.host << ":" << listenConfig.port << std::endl;
                }
        }
}

bool HttpServer::initializeServers()
{
        bool success = true;
        for (size_t i = 0; i < config.servers.size(); ++i)
        {
                try
                {
                        startServer(config.servers[i]);
                }
                catch (const std::exception &e)
                {
                        std::cerr << "Failed to start server " << (i + 1) << ": " << e.what() << std::endl;
                        success = false;
                }
        }
        return success;
}

void HttpServer::cleanup()
{
        socketManager.closeAllSockets();
}
