#include "EventLoop.hpp"

#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

EventLoop::EventLoop() : epollFd(-1), isInitialized(false)
{
}

EventLoop::~EventLoop()
{
    if (isInitialized && epollFd != -1)
    {
        close(epollFd);
    }
}

bool EventLoop::initialize()
{
    if (isInitialized)
    {
        return true;
    }

    epollFd = epoll_create(1);
    if (epollFd < 0)
    {
        std::cerr << "Failed to create epoll instance: " << strerror(errno) << std::endl;
        return false;
    }
    isInitialized = true;
    return true;
}

int EventLoop::wait(epoll_event *events, int maxEvents, int timeout)
{
    return epoll_wait(epollFd, events, maxEvents, timeout);
}

bool EventLoop::add(int fd, uint32_t events)
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

bool EventLoop::remove(int fd)
{
    if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL) < 0)
    {
        if (errno != EBADF)
        {
            std::cerr << "Failed to remove file descriptor " << fd << " from epoll: " << strerror(errno) << std::endl;
        }
        return false;
    }
    return true;
}

bool EventLoop::modify(int fd, uint32_t events)
{
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
