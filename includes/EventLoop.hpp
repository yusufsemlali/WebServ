#pragma once

#include "SocketManager.hpp"
#include "Config.hpp"
#include <vector>
#include <stdint.h>

class EventLoop
{
public:
    EventLoop(SocketManager &socketManager, const Config &config);
    ~EventLoop();

    // Main event loop
    bool initialize();
    void run();
    void stop();
    void cleanup();  // Add cleanup to public interface
    
    // Event handling
    void handleEvents();
    void handleNewConnection(int serverFd);
    void handleClientRead(int clientFd);
    void handleClientWrite(int clientFd);
    void handleClientError(int clientFd);
    
    // Timeout management
    void checkTimeouts();
    void setConnectionTimeout(int seconds);
    
    // Configuration
    bool isRunning() const;
    void setMaxEvents(int maxEvents);

private:
    SocketManager &socketManager;
    const Config &config;
    
    int epollFd;  // For Linux (epoll)
    bool running;
    int connectionTimeout;
    int maxEvents;
    
    static const int DEFAULT_TIMEOUT = 30;
    static const int DEFAULT_MAX_EVENTS = 64;
    
    // Platform-specific event handling
    bool initializeEpoll();
    bool addToEpoll(int fd, uint32_t events);
    bool removeFromEpoll(int fd);
    bool modifyEpoll(int fd, uint32_t events);
    
    // Helper methods
    bool isServerSocket(int fd) const;
};
