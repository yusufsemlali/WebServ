#ifndef CGIOPERATION_HPP
#define CGIOPERATION_HPP

#include "AsyncOperation.hpp"
#include "HttpRequest.hpp"
#include <string>
#include <sys/types.h>

/**
 * Non-blocking CGI operation implementation.
 * Handles CGI script execution without blocking the main server thread.
 */
class CgiOperation : public AsyncOperation
{
public:
    CgiOperation(const std::string& scriptPath, const std::string& interpreterPath, 
                 const HttpRequest& request, const std::string& documentRoot = "./www");
    virtual ~CgiOperation();
    
    // AsyncOperation interface implementation
    bool isComplete() const;
    std::string getResult() const;
    int getMonitorFd() const;
    void handleData();
    bool hasError() const;
    std::string getError() const;
    void cleanup();
    
    // Force completion (for EPOLLHUP case)
    void forceCompletion();
    
private:
    // CGI process information
    pid_t childPid;
    int outputFd;      // Read from CGI stdout
    int inputFd;       // Write to CGI stdin (for POST data)
    int errorFd;       // Read from CGI stderr
    
    // Operation state
    bool completed;
    bool error;
    std::string result;
    std::string errorMessage;
    std::string postData;
    
    // CGI script information
    std::string scriptPath;
    std::string interpreterPath;
    std::string documentRoot;
    
    // Helper methods
    bool startCgiProcess(const HttpRequest& request);
    char** createCgiEnvironment(const HttpRequest& request);
    void freeCgiEnvironment(char** env);
    bool setNonBlocking(int fd);
    void readFromProcess();
    void writePostData();
    bool checkProcessStatus();
    
    // Buffer management
    static const size_t BUFFER_SIZE = 4096;
    char readBuffer[BUFFER_SIZE];
};

#endif // CGIOPERATION_HPP