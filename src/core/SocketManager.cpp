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
    (void)serverFd;
    // TODO: Accept new client connection
    return false;
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

bool SocketManager::bindAndListen(int fd, const std::string &host, int port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    // Use custom IP parsing instead of inet_pton
    unsigned long ip_addr = parseIpAddress(host);
    if (ip_addr == 0 && host != "0.0.0.0")
    {
        std::cerr << "Invalid address: " << host << std::endl;
        return false;
    }
    addr.sin_addr.s_addr = ip_addr;

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        return false;
    }

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
        std::cerr << "Failed to set socket options: " << strerror(errno) << std::endl;
        return false;
    }
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
                throw std::runtime_error("Failed to close server socket: " + std::string(strerror(errno)));
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
                throw std::runtime_error("Failed to close client socket: " + std::string(strerror(errno)));
            }
        }
        delete it->second; // Clean up ClientConnection object
    }
    clientConnections.clear();
}
