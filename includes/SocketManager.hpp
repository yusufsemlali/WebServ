#pragma once

#include <map>
#include <vector>

#include "Config.hpp"

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
    int acceptConnection(int serverFd, struct sockaddr_in &outClientAddr);
    void closeConnection(int clientFd);

    // Socket operations
    bool setNonBlocking(int fd);
    bool bindAndListen(int fd, const std::string &host, const std::string &port);

    // Getters
    const std::vector<int> &getServerSockets() const;
    const std::map<int, ClientConnection *> &getClientConnections() const;
    const Config::ServerConfig *getServerConfig(int serverFd) const;

    // Nginx-style: Get all servers for a socket (for server_name matching)
    const std::vector<const Config::ServerConfig *> &getServerConfigs(int serverFd) const;

   private:
    std::vector<int> serverSockets;
    std::map<int, ClientConnection *> clientConnections;
    std::map<int, std::vector<const Config::ServerConfig *> > serverConfigs;  // Map server FD to multiple server configs
    std::map<std::string, int> addressToSocket;                               // Map "IP:port" to socket FD for reuse

    // Helper methods
    int createSocket();
    bool setSocketOptions(int fd);
    void cleanup();
};
