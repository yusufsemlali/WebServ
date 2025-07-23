#include "EventLoop.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

#include "ClientConnection.hpp"

EventLoop::EventLoop(SocketManager &socketManager, const Config &config)
    : socketManager(socketManager),
      config(config),
      epollFd(-1),
      running(false),
      connectionTimeout(DEFAULT_TIMEOUT)
{
}

EventLoop::~EventLoop()
{
    cleanup();
}

bool EventLoop::initialize()
{
    if (!initializeEpoll())
    {
        return false;
    }
    const std::vector<int> &serverSockets = socketManager.getServerSockets();
    for (size_t i = 0; i < serverSockets.size(); ++i)
    {
        if (!addToEpoll(serverSockets[i], EPOLLIN))
        {
            std::cerr << "Failed to add server socket to epoll: " << strerror(errno) << std::endl;
            return false;
        }
    }
    return true;
}

void EventLoop::run()
{
    running = true;
    while (running)
    {
        epoll_event events[DEFAULT_MAX_EVENTS];
        int eventCount = epoll_wait(epollFd, events, DEFAULT_MAX_EVENTS, connectionTimeout * 1000);
        if (eventCount < 0)
        {
            std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
            continue;
        }
        for (int i = 0; i < eventCount; ++i)
        {
            int fd = events[i].data.fd;
            if (isServerSocket(fd))
            {
                handleNewConnection(fd);
            }
            else
            {
                if (events[i].events & (EPOLLERR | EPOLLHUP))
                {
                    handleClientError(fd);
                }
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

void EventLoop::stop()
{
    running = false;
}

void EventLoop::handleNewConnection(int serverFd)
{
    std::cout << "New connection accepted on server socket: " << serverFd << std::endl;
    int clientFd = socketManager.acceptConnection(serverFd);
    if (clientFd < 0)
    {
        std::cerr << "Failed to accept new connection on server socket: " << serverFd << std::endl;
        return;
    }

    if (!addToEpoll(clientFd, EPOLLIN | EPOLLERR | EPOLLHUP))
    {
        std::cerr << "Failed to add new client socket to epoll: " << strerror(errno) << std::endl;
        socketManager.closeConnection(clientFd);
        return;
    }
}

void EventLoop::handleClientRead(int clientFd)
{
    ClientConnection *clientConn = socketManager.getClientConnections().at(clientFd);
    if (!clientConn)
    {
        std::cerr << "Client connection not found for FD: " << clientFd << std::endl;
        return;
    }

    if (clientConn->readData())
    {
        // If data was read successfully and there's a response ready to send
        if (clientConn->isReadyToWrite())
        {
            // Enable write events for this socket
            if (!modifyEpoll(clientFd, EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP))
            {
                std::cerr << "Failed to enable write events for client FD: " << clientFd << std::endl;
            }
        }
    }
}

void EventLoop::handleClientWrite(int clientFd)
{
    ClientConnection *clientConn = socketManager.getClientConnections().at(clientFd);
    if (!clientConn)
    {
        std::cerr << "Client connection not found for FD: " << clientFd << std::endl;
        return;
    }

    if (clientConn->writeData())
    {
        // Response fully sent, disable write events
        if (!modifyEpoll(clientFd, EPOLLIN | EPOLLERR | EPOLLHUP))
        {
            std::cerr << "Failed to disable write events for client FD: " << clientFd << std::endl;
        }
    }
}

void EventLoop::handleClientError(int clientFd)
{
    std::cout << "Client error/disconnection detected for FD: " << clientFd << std::endl;

    if (!removeFromEpoll(clientFd))
    {
        std::cerr << "Failed to remove client socket from epoll: " << strerror(errno) << std::endl;
    }

    socketManager.closeConnection(clientFd);
}

void EventLoop::checkTimeouts()
{
    // TODO: Check for connection timeouts
}

void EventLoop::setConnectionTimeout(int seconds)
{
    connectionTimeout = seconds;
}

bool EventLoop::isRunning() const
{
    return running;
}

bool EventLoop::initializeEpoll()
{
    epollFd = epoll_create(1);
    if (epollFd < 0)
    {
        std::cerr << "Failed to create epoll instance: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool EventLoop::addToEpoll(int fd, uint32_t events)
{
    epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        std::cerr << "Failed to add file descriptor to epoll: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool EventLoop::removeFromEpoll(int fd)
{
    if (epollFd < 0)
    {
        std::cerr << "Cannot remove fd from epoll: epoll instance not initialized" << std::endl;
        return false;
    }
    if (fd < 0)
    {
        std::cerr << "Cannot remove invalid file descriptor from epoll" << std::endl;
        return false;
    }

    if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL) < 0)
    {
        std::cerr << "Failed to remove file descriptor " << fd << " from epoll: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool EventLoop::modifyEpoll(int fd, uint32_t events)
{
    if (epollFd < 0)
    {
        std::cerr << "Cannot modify fd in epoll: epoll instance not initialized" << std::endl;
        return false;
    }

    if (fd < 0)
    {
        std::cerr << "Cannot modify invalid file descriptor in epoll" << std::endl;
        return false;
    }

    epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event) < 0)
    {
        std::cerr << "Failed to modify file descriptor " << fd << " in epoll: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool EventLoop::isServerSocket(int fd) const
{
    const std::vector<int> &serverSockets = socketManager.getServerSockets();
    for (size_t i = 0; i < serverSockets.size(); ++i)
    {
        if (serverSockets[i] == fd)
        {
            return true;
        }
    }
    return false;
}

void EventLoop::cleanup()
{
    running = false;

    if (epollFd != -1)
    {
        std::cout << "Closing epoll file descriptor " << epollFd << "..." << std::endl;
        if (close(epollFd) < 0)
        {
            std::cerr << "Warning: Failed to close epoll file descriptor: " << strerror(errno) << std::endl;
        }
        epollFd = -1;
    }
    std::cout << "EventLoop cleanup complete." << std::endl;
}
