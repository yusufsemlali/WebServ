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
    // Create address string for this listen directive
    std::string address = listenConfig.host + ":" + listenConfig.port;
    
    std::cout << "NGINX_SOCKET: Processing listen directive: " << address << std::endl;
    
    // Check if we already have a socket for this address (nginx-style socket sharing)
    std::map<std::string, int>::iterator it = addressToSocket.find(address);
    if (it != addressToSocket.end())
    {
        // Reuse existing socket - add this server to the list
        int existingSocket = it->second;
        std::cout << "NGINX_SOCKET: Reusing existing socket " << existingSocket << " for " << address << std::endl;
        serverConfigs[existingSocket].push_back(serverConfig);
        return true;
    }
    
    // Create new socket for this address
    std::cout << "NGINX_SOCKET: Creating new socket for " << address << std::endl;
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
    
    // Store the new socket
    serverSockets.push_back(serverFd);
    addressToSocket[address] = serverFd;
    serverConfigs[serverFd].push_back(serverConfig);  // Initialize with first server
    
    std::cout << "NGINX_SOCKET: Successfully created socket " << serverFd << " for " << address << std::endl;
    return true;
}

void SocketManager::closeAllSockets()
{
    cleanup();  // Reuse the cleanup logic
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

    // Set the client socket to non-blocking mode
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
    hints.ai_flags = AI_PASSIVE;  // Use the local address

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

const Config::ServerConfig *SocketManager::getServerConfig(int serverFd) const
{
    std::map<int, std::vector<const Config::ServerConfig *> >::const_iterator it = serverConfigs.find(serverFd);
    if (it != serverConfigs.end() && !it->second.empty())
    {
        // Return the first server config (for backward compatibility)
        return it->second[0];
    }
    return NULL;  // Server FD not found
}

const std::vector<const Config::ServerConfig *> &SocketManager::getServerConfigs(int serverFd) const
{
    std::map<int, std::vector<const Config::ServerConfig *> >::const_iterator it = serverConfigs.find(serverFd);
    if (it != serverConfigs.end())
    {
        return it->second;
    }
    
    // Return empty vector if not found
    static std::vector<const Config::ServerConfig *> empty;
    return empty;
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
    serverConfigs.clear();  // Clear server config mappings

    // Close all client connections
    for (std::map<int, ClientConnection *>::iterator it = clientConnections.begin();
         it != clientConnections.end(); ++it)
    {
        std::cout << "Closing client socket " << it->first << std::endl;
        delete it->second;  // Destructor will close the FD
    }
    clientConnections.clear();
}
