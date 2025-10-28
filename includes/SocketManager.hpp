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

				bool createServerSocket(const Config::ListenConfig &listenConfig, const Config::ServerConfig *serverConfig);
				void closeAllSockets();

				int acceptConnection(int serverFd, struct sockaddr_in& outClientAddr);
				void closeConnection(int clientFd);

				bool setNonBlocking(int fd);
				bool bindAndListen(int fd, const std::string &host, const std::string &port);

				const std::vector<int> &getServerSockets() const;
				const std::map<int, ClientConnection *> &getClientConnections() const;
				const std::vector<const Config::ServerConfig *> *getServerConfigs(int serverFd) const;

			 private:
				std::vector<int> serverSockets;
				std::map<int, ClientConnection *> clientConnections;
				std::map<int, std::vector<const Config::ServerConfig *> > serverConfigs;
				std::map<std::string, int> listenAddressToSocket;

				int createSocket();
				bool setSocketOptions(int fd);
				void cleanup();
};
