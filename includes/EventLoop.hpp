#pragma once

#include <stdint.h>
#include <sys/epoll.h>

class EventLoop
{
   public:
    EventLoop();
    ~EventLoop();

    bool initialize();
    int wait(epoll_event *events, int maxEvents, int timeout);

    bool add(int fd, uint32_t events);
    bool remove(int fd);
    bool modify(int fd, uint32_t events);

   private:
    int epollFd;
    bool isInitialized;

    static const int MAX_EVENTS = 64;
};