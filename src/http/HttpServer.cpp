#include "HttpServer.hpp"

#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <memory>
#include <sstream>


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
    
    std::cout << "HTTP Server started successfully" << std::endl;
    running = true;

    static const int MAX_EVENTS = 64;
    static const int TIMEOUT_MS = 1000;

    while (running)
    {
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

    std::cout << "New connection accepted on fd: " << clientFd << " from server socket: " << serverFd << std::endl;

    // Get all server configs for this server socket (nginx-style: multiple servers per socket)
    const std::vector<const Config::ServerConfig *> &serverConfigs = socketManager.getServerConfigs(serverFd);
    if (serverConfigs.empty()) {
        std::cerr << "Error: No server configs found for server socket " << serverFd << std::endl;
        close(clientFd);
        return;
    }

    // For now, use the first server config (will be enhanced with server_name matching later)
    const Config::ServerConfig* selectedServerConfig = serverConfigs[0];
    
    std::cout << "NGINX_STYLE: Socket " << serverFd << " has " << serverConfigs.size() << " server(s)" << std::endl;
    std::cout << "NGINX_STYLE: Using first server config for now (server_name matching to be added)" << std::endl;

    connections[clientFd] = new ClientConnection(clientFd, clientAddr, requestHandler, selectedServerConfig);

    if (!eventLoop.add(clientFd, EPOLLIN))
    {
        closeConnection(clientFd);
    }
}

void HttpServer::handleClientRead(int clientFd)
{
    std::cout << "Handling read on client FD: " << clientFd << std::endl;
    
    std::map<int, ClientConnection*>::iterator it = connections.find(clientFd);
    if (it == connections.end()) 
    {
        std::cerr << "Client connection not found for FD: " << clientFd << std::endl;
        return;
    }

    ClientConnection* conn = it->second;
    if (!conn->readData())
    {
        std::cout << "Read failed, closing connection" << std::endl;
        closeConnection(clientFd);
        return;
    }

    // FIXED: Check if we have a response ready to send
    if (conn->isReadyToWrite())
    {
        std::cout << "Response ready, switching to write mode" << std::endl;
        eventLoop.modify(clientFd, EPOLLIN | EPOLLOUT);
    }
}

void HttpServer::handleClientWrite(int clientFd)
{
    std::cout << "Handling write on client FD: " << clientFd << std::endl;
    
    std::map<int, ClientConnection*>::iterator it = connections.find(clientFd);
    if (it == connections.end()) 
    {
        std::cerr << "Client connection not found for FD: " << clientFd << std::endl;
        return;
    }

    ClientConnection* conn = it->second;
    
    // FIXED: writeData() returns true when complete, false when partial
    bool writeComplete = conn->writeData();
    
    if (!conn->isConnected())
    {
        std::cout << "Connection closed during write" << std::endl;
        closeConnection(clientFd);
        return;
    }

    if (writeComplete)
    {
        std::cout << "Write complete, response sent successfully" << std::endl;
        // Response sent completely
        if (!conn->isKeepAlive())
        {
            std::cout << "Closing connection (no keep-alive)" << std::endl;
            closeConnection(clientFd);
        }
        else
        {
            // Switch back to read mode for keep-alive
            std::cout << "Keep-alive connection, switching back to read mode" << std::endl;
            eventLoop.modify(clientFd, EPOLLIN);
        }
    }
    else
    {
        // Partial write, keep EPOLLOUT to continue writing
        std::cout << "Partial write, continuing..." << std::endl;
    }
}

void HttpServer::handleClientError(int clientFd)
{
    std::cout << "Client error/hangup on fd: " << clientFd << std::endl;
    closeConnection(clientFd);
}

void HttpServer::closeConnection(int clientFd)
{
    std::cout << "Closing connection on fd: " << clientFd << std::endl;
    
    eventLoop.remove(clientFd);
    
    std::map<int, ClientConnection*>::iterator it = connections.find(clientFd);
    if (it != connections.end())
    {
        delete it->second;  // This calls ClientConnection destructor which closes the socket
        connections.erase(it);
    }
    
    std::cout << "Connection closed successfully" << std::endl;
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
    // TODO: Implement timeout logic
    // For now, this can be empty
    // Check for connection timeouts
}
