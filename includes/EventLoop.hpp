// #pragma once

// #include <stdint.h>
// #include <sys/epoll.h>

// #include "Config.hpp"
// #include "SocketManager.hpp"

// class EventLoop
// {
//    public:
//     EventLoop(SocketManager &socketManager, const Config &config);
//     ~EventLoop();

//     // Main event loop
//     bool initialize();
//     void run();
//     void stop();
//     void cleanup();

//     // Event handling
//     int poll(epoll_event *events, int maxEvents);
//     void handleNewConnection(int serverFd);
//     void handleClientRead(int clientFd);
//     void handleClientWrite(int clientFd);
//     void handleClientError(int clientFd);

//     // Timeout management
//     void checkTimeouts();
//     void setConnectionTimeout(int seconds);

//     // Configuration
//     bool isRunning() const;

//    private:
//     SocketManager &socketManager;
//     const Config &config;

//     int epollFd;  // For Linux (epoll)
//     bool running;
//     int connectionTimeout;

//     static const int DEFAULT_TIMEOUT = 30;
//     static const int DEFAULT_MAX_EVENTS = 64;

//     // Platform-specific event handling
//     bool initializeEpoll();
//     bool addToEpoll(int fd, uint32_t events);
//     bool removeFromEpoll(int fd);
//     bool modifyEpoll(int fd, uint32_t events);

//     // Helper methods
//     bool isServerSocket(int fd) const;
// };

#pragma once

#include <stdint.h>
#include <sys/epoll.h>

// EventLoop is now a pure epoll wrapper and has no knowledge of the application.
// It no longer needs SocketManager or Config.
class EventLoop
{
   public:
    EventLoop();
    ~EventLoop();

    // --- Core Epoll Management ---

    // Initializes the epoll instance.
    bool initialize();

    // Waits for events on the epoll instance.
    // This replaces the old poll() method for clarity.
    int wait(epoll_event *events, int maxEvents, int timeout);

    // Adds a file descriptor to epoll.
    bool add(int fd, uint32_t events);

    // Removes a file descriptor from epoll.
    bool remove(int fd);

    // Modifies the events for an existing file descriptor.
    bool modify(int fd, uint32_t events);

   private:
    int epollFd;  // The epoll file descriptor.
    bool isInitialized;

    // Renamed for clarity.
    static const int MAX_EVENTS = 64;
};