#ifndef ASYNCOPERATION_HPP
#define ASYNCOPERATION_HPP

#include <string>


class AsyncOperation 
{
public:
    virtual ~AsyncOperation() {}
    
    virtual bool isComplete() const = 0;
    
    virtual std::string getResult() const = 0;
    
    virtual int getMonitorFd() const = 0;
    
    virtual void handleData() = 0;
    
    virtual bool hasError() const = 0;

    virtual std::string getError() const = 0;
    
    virtual void cleanup() = 0;
};

#endif