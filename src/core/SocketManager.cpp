#include "SocketManager.hpp"

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

#include "ClientConnection.hpp"
#include "utiles.hpp"

SocketManager::SocketManager()
{
}

SocketManager::~SocketManager()
{
    cleanup();
}

bool SocketManager::createServerSocket(const Config::ListenConfig &listenConfig, const Config::ServerConfig *serverConfig)
{
    std::string listenKey = listenConfig.host + ":" + listenConfig.port;
    std::map<std::string, int>::iterator it = listenAddressToSocket.find(listenKey);
    
    if (it != listenAddressToSocket.end())
    {
        serverConfigs[it->second].push_back(serverConfig);
        return true;
    }
    
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
    serverConfigs[serverFd].push_back(serverConfig);
    listenAddressToSocket[listenKey] = serverFd;

    return true;
}

void SocketManager::closeAllSockets()
{
    cleanup(); 
}

int SocketManager::acceptConnection(int serverFd, struct sockaddr_in &outClientAddr)
{
    socklen_t addrLen = sizeof(outClientAddr);
    int clientFd = accept(serverFd, (struct sockaddr *)&outClientAddr, &addrLen);
    if (clientFd < 0)
    {
        std::cerr << "Failed to accept connection: " << strerror(errno) << std::endl;
        return -1;
    }

    if (!setNonBlocking(clientFd))
    {
        std::cerr << "Failed to set client socket to non-blocking mode" << std::endl;
        close(clientFd);
        return -1;
    }
    return clientFd;
}

void SocketManager::closeConnection(int clientFd)
{
    std::map<int, ClientConnection *>::iterator it = clientConnections.find(clientFd);

    if (it == clientConnections.end())
    {
        std::cerr << "Client connection not found for FD: " << clientFd << std::endl;
        return;
    }

    ClientConnection *clientConn = it->second;
    clientConnections.erase(it);

    delete clientConn;
}

bool SocketManager::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
        std::cerr << "Failed to get socket flags: " << strerror(errno) << std::endl;
        return false;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        std::cerr << "Failed to set socket to non-blocking: " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

bool SocketManager::bindAndListen(int fd, const std::string &host, const std::string &port)
{
    struct addrinfo hints;
    struct addrinfo *servinfo;
    ft_memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; 

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

const std::vector<const Config::ServerConfig *> *SocketManager::getServerConfigs(int serverFd) const
{
    std::map<int, std::vector<const Config::ServerConfig *> >::const_iterator it = serverConfigs.find(serverFd);
    if (it != serverConfigs.end())
    {
        return &it->second;
    }
    return NULL;
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
    if (!setNonBlocking(fd))
    {
        std::cerr << "Failed to set server socket to non-blocking mode" << std::endl;
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
    serverConfigs.clear();
    listenAddressToSocket.clear();

    // Close all client connections
    for (std::map<int, ClientConnection *>::iterator it = clientConnections.begin();
         it != clientConnections.end(); ++it)
    {
        std::cout << "Closing client socket " << it->first << std::endl;
        delete it->second;  // Destructor will close the FD
    }
    clientConnections.clear();
}
