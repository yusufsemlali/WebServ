#include "SocketManager.hpp"
#include "ClientConnection.hpp"
#include "utiles.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <netdb.h>

SocketManager::SocketManager() {}

SocketManager::~SocketManager()
{
    cleanup();
}

bool SocketManager::createServerSocket(const Config::ListenConfig &listenConfig)
{
    int serverFd = createSocket();
    if (serverFd < 0)
    {
        return false;
    }
    if (!bindAndListen(serverFd, listenConfig.host, listenConfig.port))
    {
        close(serverFd);
        return false;
    }
    serverSockets.push_back(serverFd);
    return true;
}

void SocketManager::closeAllSockets()
{
    cleanup(); // Reuse the cleanup logic
}

bool SocketManager::acceptConnection(int serverFd)
{
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &addrLen);
    if (clientFd < 0)
    {
        std::cerr << "Failed to accept connection: " << strerror(errno) << std::endl;
        return false;
    }

    // Create a new ClientConnection object
    ClientConnection *newClient = new ClientConnection(clientFd, clientAddr);

    // Store the client connection
    clientConnections[clientFd] = newClient;
    return true;
}

void SocketManager::closeConnection(int clientFd)
{
    (void)clientFd;
    // TODO: Close client connection
}

bool SocketManager::setNonBlocking(int fd)
{
    (void)fd;
    // TODO: Set socket to non-blocking mode
    return false;
}

bool SocketManager::bindAndListen(int fd, const std::string &host, const std::string &port)
{
    struct addrinfo hints;
    struct addrinfo *servinfo;
    ft_memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // Use the local address

    int status = getaddrinfo(host.c_str(), port.c_str(), &hints, &servinfo);
    if (status != 0)
    {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
        return false;
    }

    if (bind(fd, servinfo->ai_addr, servinfo->ai_addrlen) < 0)
    {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        freeaddrinfo(servinfo);
        return false;
    }

    freeaddrinfo(servinfo);

    if (listen(fd, SOMAXCONN) < 0)
    {
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

const std::vector<int> &SocketManager::getServerSockets() const
{
    return serverSockets;
}

const std::map<int, ClientConnection *> &SocketManager::getClientConnections() const
{
    return clientConnections;
}

int SocketManager::createSocket()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return -1;
    }
    if (!setSocketOptions(fd))
    {
        std::cerr << "Failed to set socket options: " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }
    return fd;
}

bool SocketManager::setSocketOptions(int fd)
{
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "Failed to set SO_REUSEADDR: " << strerror(errno) << std::endl;
        return false;
    }
    
    // Also set SO_REUSEPORT for better handling of multiple processes
    // if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    // {
    //     std::cerr << "Failed to set SO_REUSEPORT: " << strerror(errno) << std::endl;
    //     return false;
    // }
    
    return true;
}

void SocketManager::cleanup()
{
    std::cout << "Cleaning up SocketManager..." << std::endl;

    // Close all server sockets
    for (size_t i = 0; i < serverSockets.size(); ++i)
    {
        if (serverSockets[i] != -1)
        {
            std::cout << "Closing server socket " << serverSockets[i] << std::endl;
            if (close(serverSockets[i]) < 0)
            {
                std::cerr << "Warning: Failed to close server socket: " << strerror(errno) << std::endl;
            }
        }
    }
    serverSockets.clear();

    // Close all client connections
    for (std::map<int, ClientConnection *>::iterator it = clientConnections.begin();
         it != clientConnections.end(); ++it)
    {
        if (it->first != -1)
        {
            std::cout << "Closing client socket " << it->first << std::endl;
            if (close(it->first) < 0)
            {
                std::cerr << "Warning: Failed to close client socket: " << strerror(errno) << std::endl;
            }
        }
        delete it->second; // Clean up ClientConnection object
    }
    clientConnections.clear();
}
