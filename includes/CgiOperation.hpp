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
                 const HttpRequest& request, const std::string& documentRoot = "./www",
                 const std::string& serverPort = "8080", const std::string& clientAddr = "127.0.0.1",
                 size_t clientMaxBodySize = 0);
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
    std::string serverPort;
    std::string clientAddress;
    size_t clientMaxBodySize;
    
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