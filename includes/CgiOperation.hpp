#ifndef CGIOPERATION_HPP
#define CGIOPERATION_HPP

#include "AsyncOperation.hpp"
#include "HttpRequest.hpp"
#include <string>
#include <sys/types.h>


class CgiOperation : public AsyncOperation
{
public:
    CgiOperation(const std::string& scriptPath, const std::string& interpreterPath, 
                 const HttpRequest& request, const std::string& documentRoot = "./www");
    virtual ~CgiOperation();
    
    bool isComplete() const;
    std::string getResult() const;
    int getMonitorFd() const;
    void handleData();
    bool hasError() const;
    std::string getError() const;
    void cleanup();
    
    void forceCompletion();
    
private:
    pid_t childPid;
    int outputFd;
    int inputFd;
    int errorFd;
    
    bool completed;
    bool error;
    std::string result;
    std::string errorMessage;
    std::string postData;
    
    std::string scriptPath;
    std::string interpreterPath;
    std::string documentRoot;
    
    bool startCgiProcess(const HttpRequest& request);
    char** createCgiEnvironment(const HttpRequest& request);
    void freeCgiEnvironment(char** env);
    bool setNonBlocking(int fd);
    void readFromProcess();
    void writePostData();
    bool checkProcessStatus();
    
    static const size_t BUFFER_SIZE = 4096;
    char readBuffer[BUFFER_SIZE];
};

#endif