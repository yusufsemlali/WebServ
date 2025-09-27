#ifndef ASYNCOPERATION_HPP
#define ASYNCOPERATION_HPP

#include <string>

/**
 * Simple interface for asynchronous operations that can be monitored by epoll.
 * This allows clean separation between the operation logic and socket management.
 */
class AsyncOperation 
{
public:
    virtual ~AsyncOperation() {}
    
    // Check if the operation has completed
    virtual bool isComplete() const = 0;
    
    // Get the result of the operation (only call if isComplete() returns true)
    virtual std::string getResult() const = 0;
    
    // Get the file descriptor that should be monitored in epoll
    virtual int getMonitorFd() const = 0;
    
    // Handle data available on the monitor FD (called by event loop)
    virtual void handleData() = 0;
    
    // Get error status if operation failed
    virtual bool hasError() const = 0;
    virtual std::string getError() const = 0;
    
    // Cleanup resources
    virtual void cleanup() = 0;
};

#endif // ASYNCOPERATION_HPP