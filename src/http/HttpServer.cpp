#include "HttpServer.hpp"
#include "AsyncOperation.hpp"
#include "CgiOperation.hpp"

#include <netinet/in.h>

#include <cerrno>
#include <cstring>
#include <iostream>

HttpServer::HttpServer(Config& config)
    : config(config),
      socketManager(),
      eventLoop(),
      requestHandler(config, socketManager),
      running(false)
{
}

HttpServer::~HttpServer()
{
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
#ifdef VERBOSE_LOGGING
            std::cout << "Event on FD " << fd << " with events: " << events[i].events << std::endl;
#endif
            
            if (isServerSocket(fd))
            {
                handleNewConnection(fd);
            }
            else if (isCgiSocket(fd))
            {
                // Handle CGI events
                if (events[i].events & (EPOLLERR | EPOLLHUP))
                {
                    handleCgiError(fd);
                }
                else if (events[i].events & EPOLLIN)
                {
                    handleCgiRead(fd);
                }
            }
            else
            {
                // Handle client events
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
    }

#ifdef VERBOSE_LOGGING
    std::cout << "Server listening on sockets : ";
    for(size_t i = 0; i < serverSockets.size(); ++i)
    {
        std::cout << serverSockets[i] << ", ";
    }
    std::cout << std::endl;
#endif

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

#ifdef VERBOSE_LOGGING
    std::cout << "New connection accepted on fd: " << clientFd << std::endl;
#endif

    connections[clientFd] = new ClientConnection(clientFd, clientAddr, requestHandler);
    
    connections[clientFd]->setServerFd(serverFd);    

    if (!eventLoop.add(clientFd, EPOLLIN))
    {
        closeConnection(clientFd);
    }
}

void HttpServer::handleClientRead(int clientFd)
{
    
    std::map<int, ClientConnection*>::iterator it = connections.find(clientFd);
    if (it == connections.end()) 
    {
        std::cerr << "Client connection not found for FD: " << clientFd << std::endl;
        return;
    }

    ClientConnection* conn = it->second;
    if (!conn->readData())
    {
        closeConnection(clientFd);
        return;
    }

    if (conn->hasPendingOperation())
    {
        AsyncOperation* op = conn->getPendingOperation();
        int cgiFd = op->getMonitorFd();
        
        if (cgiConnections.find(cgiFd) == cgiConnections.end())
        {
            if (eventLoop.add(cgiFd, EPOLLIN))
            {
                cgiConnections[cgiFd] = conn; 
                op->handleData();
            }
            else
            {
                std::cerr << "HttpServer: Failed to add CGI fd " << cgiFd << " to event loop!" << std::endl;
                conn->completePendingOperation();
            }
        }
    }
    else if (conn->isReadyToWrite())
    {
        eventLoop.modify(clientFd, EPOLLIN | EPOLLOUT);
    }
}

void HttpServer::handleClientWrite(int clientFd)
{
    
    std::map<int, ClientConnection*>::iterator it = connections.find(clientFd);
    if (it == connections.end()) 
    {
        std::cerr << "Client connection not found for FD: " << clientFd << std::endl;
        return;
    }

    ClientConnection* conn = it->second;
    
    bool writeComplete = conn->writeData();
    
    if (!conn->isConnected())
    {
        closeConnection(clientFd);
        return;
    }

    if (writeComplete)
    {
        if (!conn->isKeepAlive())
        {
            closeConnection(clientFd);
        }
        else
        {
            eventLoop.modify(clientFd, EPOLLIN);
        }
    }
}

void HttpServer::handleClientError(int clientFd)
{
    closeConnection(clientFd);
}

void HttpServer::closeConnection(int clientFd)
{
    eventLoop.remove(clientFd);
    
    std::map<int, ClientConnection*>::iterator it = connections.find(clientFd);
    if (it != connections.end())
    {
        delete it->second; 
        connections.erase(it);
    }
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

bool HttpServer::isCgiSocket(int fd) const
{
    return cgiConnections.find(fd) != cgiConnections.end();
}

void HttpServer::handleCgiRead(int cgiFd)
{
    
    std::map<int, ClientConnection*>::iterator it = cgiConnections.find(cgiFd);
    if (it == cgiConnections.end())
    {
        std::cerr << "CGI FD not found in cgiConnections map" << std::endl;
        return;
    }
    
    ClientConnection* conn = it->second;
    AsyncOperation* op = conn->getPendingOperation();
    
    if (!op)
    {
        std::cerr << "No pending operation for CGI FD: " << cgiFd << std::endl;
        return;
    }
    
    op->handleData();
    
    if (op->isComplete())
    {
        eventLoop.remove(cgiFd);
        cgiConnections.erase(it);
        
        conn->completePendingOperation();
        
        if (conn->canWrite())
        {
            eventLoop.modify(conn->getSocketFd(), EPOLLIN | EPOLLOUT);
        }
    }
}

void HttpServer::handleCgiError(int cgiFd)
{
    
    std::map<int, ClientConnection*>::iterator it = cgiConnections.find(cgiFd);
    if (it != cgiConnections.end())
    {
        ClientConnection* conn = it->second;
        AsyncOperation* op = conn->getPendingOperation();
        
        if (op) {
            op->handleData();
            
            CgiOperation* cgiOp = dynamic_cast<CgiOperation*>(op);
            if (cgiOp && !op->isComplete()) {
                cgiOp->forceCompletion();
            }
        }
        
        conn->completePendingOperation();
        
        eventLoop.remove(cgiFd);
        cgiConnections.erase(it);
        
        if (conn->canWrite())
        {
            eventLoop.modify(conn->getSocketFd(), EPOLLIN | EPOLLOUT);
        }
    }
}
