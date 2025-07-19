#pragma once

#include "Config.hpp"
#include <vector>
#include <map>

class ServerSocket;
class ClientConnection;

class SocketManager
{
public:
    SocketManager();
    ~SocketManager();

    // Server socket management
    bool createServerSocket(const Config::ListenConfig &listenConfig, const Config::ServerConfig *serverConfig);
    void closeAllSockets();

    // Client connection management
    int acceptConnection(int serverFd);
    void closeConnection(int clientFd);

    // Socket operations
    bool setNonBlocking(int fd);
    bool bindAndListen(int fd, const std::string &host, const std::string &port);

    // Getters
    const std::vector<int> &getServerSockets() const;
    const std::map<int, ClientConnection *> &getClientConnections() const;
    const Config::ServerConfig *getServerConfig(int serverFd) const;

private:
    std::vector<int> serverSockets;
    std::map<int, ClientConnection *> clientConnections;
    std::map<int, const Config::ServerConfig *> serverConfigs; // Map server FD to server config

    // Helper methods
    int createSocket();
    bool setSocketOptions(int fd);
    void cleanup();
};
