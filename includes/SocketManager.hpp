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
    bool createServerSocket(const Config::ListenConfig &listenConfig);
    void closeAllSockets();

    // Client connection management
    bool acceptConnection(int serverFd);
    void closeConnection(int clientFd);

    // Socket operations
    bool setNonBlocking(int fd);
    bool bindAndListen(int fd, const std::string &host, int port);

    // Getters
    const std::vector<int> &getServerSockets() const;
    const std::map<int, ClientConnection *> &getClientConnections() const;

private:
    std::vector<int> serverSockets;
    std::map<int, ClientConnection *> clientConnections;

    // Helper methods
    int createSocket();
    bool setSocketOptions(int fd);
    void cleanup();
};
