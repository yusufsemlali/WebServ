
#include "HttpServer.hpp"

#include <netinet/in.h>

#include <cerrno>
#include <cstring>
#include <iostream>

#include "core.hpp"

HttpServer::HttpServer(Config& config)
    : config(config),
      socketManager(),
      eventLoop(),
      requestHandler(config),
      running(false)
{
}

HttpServer::~HttpServer()
{
    // Cleanup connections
    for (std::map<int, ClientConnection*>::iterator it = connections.begin(); it != connections.end(); ++it)
    {
        socketManager.closeConnection(it->first);
        delete it->second;
    }
    connections.clear();
    socketManager.closeAllSockets();
}

void HttpServer::run()
{
    if (!initializeServers())
    {
        throw std::runtime_error("Failed to initialize servers");
    }
    //
    std::cout << "HTTP Server started successfully" << std::endl;
    running = true;

    static const int MAX_EVENTS = 64;
    static const int TIMEOUT_MS = 1000;

    while (running)
    {
        if (shutdown_requested)
        {
            stop();
        };

        epoll_event events[MAX_EVENTS];
        int eventCount = eventLoop.wait(events, MAX_EVENTS, TIMEOUT_MS);

        if (eventCount < 0)
        {
            if (errno == EINTR) continue;
            std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
            break;
        }

        for (int i = 0; i < eventCount; ++i)
        {
            int fd = events[i].data.fd;
            if (isServerSocket(fd))
            {
                handleNewConnection(fd);
            }
            else
            {
                if (events[i].events & (EPOLLERR | EPOLLHUP))
                {
                    handleClientError(fd);
                }
                else
                {
                    if (events[i].events & EPOLLIN)
                    {
                        handleClientRead(fd);
                    }
                    if (events[i].events & EPOLLOUT)
                    {
                        handleClientWrite(fd);
                    }
                }
            }
        }
        checkTimeouts();
    }
}

void HttpServer::stop()
{
    running = false;
}

bool HttpServer::initializeServers()
{
    if (!eventLoop.initialize())
    {
        return false;
    }

    const std::vector<Config::ServerConfig>& serverConfigs = config.servers;
    for (size_t i = 0; i < serverConfigs.size(); ++i)
    {
        const std::vector<Config::ListenConfig>& listenConfigs = serverConfigs[i].listenConfigs;
        for (size_t j = 0; j < listenConfigs.size(); ++j)
        {
            if (!socketManager.createServerSocket(listenConfigs[j], &serverConfigs[i]))
            {
                std::cerr << "Failed to create server socket for host "
                          << listenConfigs[j].host << ":" << listenConfigs[j].port << std::endl;
                return false;
            }
        }
    }

    const std::vector<int>& serverSockets = socketManager.getServerSockets();
    for (size_t i = 0; i < serverSockets.size(); ++i)
    {
        if (!eventLoop.add(serverSockets[i], EPOLLIN))
        {
            std::cerr << "Failed to add server socket " << serverSockets[i] << " to event loop" << std::endl;
            return false;
        }
        std::cout << "Server listening on socket " << serverSockets[i] << std::endl;
    }
    return true;
}

void HttpServer::handleNewConnection(int serverFd)
{
    struct sockaddr_in clientAddr;
    int clientFd = socketManager.acceptConnection(serverFd, clientAddr);
    if (clientFd < 0)
    {
        return;
    }

    std::cout << "New connection accepted on fd: " << clientFd << std::endl;

    connections[clientFd] = new ClientConnection(clientFd, clientAddr, requestHandler);
    // ClientConnection(int socketFd, const struct sockaddr_in& clientAddr, HttpResponse& currentResponse);

    if (!eventLoop.add(clientFd, EPOLLIN))
    {
        closeConnection(clientFd);
    }
}

void HttpServer::handleClientRead(int clientFd)
{
    std::map<int, ClientConnection*>::iterator it = connections.find(clientFd);
    if (it == connections.end()) return;

    ClientConnection* conn = it->second;
    if (!conn->readData())
    {
        closeConnection(clientFd);
        return;
    }

    // if (conn->isReadyToWrite())
    {
        eventLoop.modify(clientFd, EPOLLIN | EPOLLOUT);
    }
}

void HttpServer::handleClientWrite(int clientFd)
{
    std::map<int, ClientConnection*>::iterator it = connections.find(clientFd);
    if (it == connections.end()) return;

    ClientConnection* conn = it->second;
    if (!conn->writeData())
    {
        closeConnection(clientFd);
        return;
    }

    if (!conn->isReadyToWrite())
    {
        eventLoop.modify(clientFd, EPOLLIN);
        if (!conn->isConnected())
        {
            closeConnection(clientFd);
        }
    }
}

void HttpServer::handleClientError(int clientFd)
{
    std::cout << "Client error/hangup on fd: " << clientFd << std::endl;
    closeConnection(clientFd);
}

void HttpServer::closeConnection(int clientFd)
{
    eventLoop.remove(clientFd);
    socketManager.closeConnection(clientFd);

    std::map<int, ClientConnection*>::iterator it = connections.find(clientFd);
    if (it != connections.end())
    {
        delete it->second;
        connections.erase(it);
    }
    std::cout << "Closed connection on fd: " << clientFd << std::endl;
}

bool HttpServer::isServerSocket(int fd) const
{
    const std::vector<int>& serverSockets = socketManager.getServerSockets();
    for (size_t i = 0; i < serverSockets.size(); ++i)
    {
        if (serverSockets[i] == fd)
        {
            return true;
        }
    }
    return false;
}

void HttpServer::checkTimeouts()
{
    // TODO: Implement timeout logic.
}
