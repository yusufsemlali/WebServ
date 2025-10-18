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

    std::cout << "New connection accepted on fd: " << clientFd << std::endl;

    connections[clientFd] = new ClientConnection(clientFd, clientAddr, requestHandler);
    
    connections[clientFd]->setServerFd(serverFd);    

    if (!eventLoop.add(clientFd, EPOLLIN))
    {
        closeConnection(clientFd);
    }
}

void HttpServer::handleClientRead(int clientFd)
{
#ifdef VERBOSE_LOGGING
    std::cout << "Handling read on client FD: " << clientFd << std::endl;
#endif
    
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

    // Check if we have an async operation pending
    if (conn->hasPendingOperation())
    {
        AsyncOperation* op = conn->getPendingOperation();
        int cgiFd = op->getMonitorFd();
        
        std::cout << "Async operation started, monitoring CGI FD: " << cgiFd << std::endl;
        
        // Register CGI FD with event loop
        if (eventLoop.add(cgiFd, EPOLLIN))
        {
            // Map CGI FD to client connection
            cgiConnections[cgiFd] = conn;
            std::cout << "CGI FD " << cgiFd << " registered for monitoring" << std::endl;
        }
        else
        {
            std::cerr << "Failed to add CGI FD to event loop" << std::endl;
            // Cleanup the operation
            conn->completePendingOperation();
        }
    }
    // FIXED: Check if we have a response ready to send
    else if (conn->isReadyToWrite())
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
        delete it->second; 
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

bool HttpServer::isCgiSocket(int fd) const
{
    return cgiConnections.find(fd) != cgiConnections.end();
}

void HttpServer::handleCgiRead(int cgiFd)
{
    std::cout << "Handling CGI read on FD: " << cgiFd << std::endl;
    
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
    
    // Let the operation handle the data
    op->handleData();
    
    // Check if operation completed
    if (op->isComplete())
    {
        std::cout << "CGI operation completed on FD: " << cgiFd << std::endl;
        
        // Remove CGI FD from event loop
        eventLoop.remove(cgiFd);
        cgiConnections.erase(it);
        
        // Complete the operation on the client connection
        conn->completePendingOperation();
        
        // Check if response is ready to write
        if (conn->canWrite())
        {
            std::cout << "CGI response ready, switching to write mode" << std::endl;
            eventLoop.modify(conn->getSocketFd(), EPOLLIN | EPOLLOUT);
        }
    }
}

void HttpServer::handleCgiError(int cgiFd)
{
    std::cout << "CGI error/hangup on FD: " << cgiFd << std::endl;
    
    std::map<int, ClientConnection*>::iterator it = cgiConnections.find(cgiFd);
    if (it != cgiConnections.end())
    {
        ClientConnection* conn = it->second;
        AsyncOperation* op = conn->getPendingOperation();
        
        if (op) {
            // EPOLLHUP might just mean CGI finished normally
            // Let the operation handle any remaining data and check status
            op->handleData();
            
            // Cast to CgiOperation to call forceCompletion
            CgiOperation* cgiOp = dynamic_cast<CgiOperation*>(op);
            if (cgiOp && !op->isComplete()) {
                std::cout << "CGI operation not complete after EPOLLHUP, forcing completion" << std::endl;
                cgiOp->forceCompletion();
            }
        }
        
        // Complete the operation
        conn->completePendingOperation();
        
        // Remove from event loop and cleanup
        eventLoop.remove(cgiFd);
        cgiConnections.erase(it);
        
        // Check if we need to send response
        if (conn->canWrite())
        {
            std::cout << "CGI finished, switching to write mode for response" << std::endl;
            eventLoop.modify(conn->getSocketFd(), EPOLLIN | EPOLLOUT);
        }
    }
}

void HttpServer::checkTimeouts()
{
    // TODO: Implement timeout logic if needed
    // For now, this can be empty
}