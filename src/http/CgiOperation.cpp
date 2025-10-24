#include "CgiOperation.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

CgiOperation::CgiOperation(const std::string& scriptPath, const std::string& interpreterPath, 
                           const HttpRequest& request, const std::string& documentRoot,
                           const std::string& serverPort, const std::string& clientAddr)
    : childPid(-1), outputFd(-1), inputFd(-1), errorFd(-1),
      completed(false), error(false), 
      scriptPath(scriptPath), interpreterPath(interpreterPath), documentRoot(documentRoot),
      serverPort(serverPort), clientAddress(clientAddr)
{
    
    if (request.getMethod() == "POST") {
        postData = request.getBody();
    }
    
    if (!startCgiProcess(request)) {
        error = true;
        completed = true;
        errorMessage = "Failed to start CGI process";
    }
}

CgiOperation::~CgiOperation()
{
    cleanup();
}

bool CgiOperation::isComplete() const
{
    return completed;
}

std::string CgiOperation::getResult() const
{
    return result;
}

int CgiOperation::getMonitorFd() const
{
    return outputFd;
}

void CgiOperation::handleData()
{
    if (completed) return;
    
    std::cout << "CgiOperation: handleData called, reading from process" << std::endl;
    
    // Write POST data first if we haven't yet
    if (!postData.empty() && inputFd >= 0) {
        std::cout << "CgiOperation: Writing POST data (" << postData.size() << " bytes remaining)" << std::endl;
        writePostData();
    }
    
    // Then read from process
    readFromProcess();
    
    if (checkProcessStatus()) {
        std::cout << "CgiOperation: Process completed" << std::endl;
        completed = true;
    }
}

bool CgiOperation::hasError() const
{
    return error;
}

std::string CgiOperation::getError() const
{
    return errorMessage;
}

void CgiOperation::cleanup()
{
    if (outputFd >= 0) {
        close(outputFd);
        outputFd = -1;
    }
    if (inputFd >= 0) {
        close(inputFd);
        inputFd = -1;
    }
    if (errorFd >= 0) {
        close(errorFd);
        errorFd = -1;
    }
    
    if (childPid > 0) {
        int status;
        if (waitpid(childPid, &status, WNOHANG) == 0) {
            kill(childPid, SIGTERM);
            waitpid(childPid, &status, 0);
        }
        childPid = -1;
    }
}

bool CgiOperation::startCgiProcess(const HttpRequest& request)
{
    int pipeStdout[2], pipeStdin[2], pipeStderr[2];
    
    if (pipe(pipeStdout) == -1 || pipe(pipeStdin) == -1 || pipe(pipeStderr) == -1) {
        std::cerr << "CgiOperation: Failed to create pipes: " << strerror(errno) << std::endl;
        return false;
    }
    
    childPid = fork();
    if (childPid == -1) {
        std::cerr << "CgiOperation: Fork failed: " << strerror(errno) << std::endl;
        close(pipeStdout[0]); close(pipeStdout[1]);
        close(pipeStdin[0]); close(pipeStdin[1]);
        close(pipeStderr[0]); close(pipeStderr[1]);
        return false;
    }
    
    if (childPid == 0) {
        
        dup2(pipeStdout[1], STDOUT_FILENO);
        dup2(pipeStdin[0], STDIN_FILENO);
        dup2(pipeStderr[1], STDERR_FILENO);
        
        close(pipeStdout[0]); close(pipeStdout[1]);
        close(pipeStdin[0]); close(pipeStdin[1]);
        close(pipeStderr[0]); close(pipeStderr[1]);
        
        char** env = createCgiEnvironment(request);
        
        if (interpreterPath.empty()) {
            char* argv[] = {const_cast<char*>(scriptPath.c_str()), NULL};
            execve(scriptPath.c_str(), argv, env);
        } else {
            char* argv[] = {const_cast<char*>(interpreterPath.c_str()), const_cast<char*>(scriptPath.c_str()), NULL};
            execve(interpreterPath.c_str(), argv, env);
        }
        
        freeCgiEnvironment(env);
        std::cerr << "CgiOperation: exec failed: " << strerror(errno) << std::endl;
        exit(1);
    }
    
    
    close(pipeStdout[1]);
    close(pipeStdin[0]);
    close(pipeStderr[1]);
    
    outputFd = pipeStdout[0];
    inputFd = pipeStdin[1];
    errorFd = pipeStderr[0];
    
    if (!setNonBlocking(outputFd) || !setNonBlocking(inputFd) || !setNonBlocking(errorFd)) {
        std::cerr << "CgiOperation: Failed to set non-blocking mode" << std::endl;
        return false;
    }
    return true;
}

char** CgiOperation::createCgiEnvironment(const HttpRequest& request)
{
    std::vector<std::string> envVars;
    
    envVars.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envVars.push_back("SERVER_SOFTWARE=WebServ/1.0");
    envVars.push_back("SERVER_PROTOCOL=HTTP/1.1");
    envVars.push_back("SERVER_NAME=localhost");
    envVars.push_back("SERVER_PORT=" + serverPort);
    envVars.push_back("REQUEST_METHOD=" + request.getMethod());
    envVars.push_back("SCRIPT_NAME=" + request.getUri());
    envVars.push_back("SCRIPT_FILENAME=" + scriptPath);
    envVars.push_back("QUERY_STRING=" + request.getQuery());
    envVars.push_back("REMOTE_ADDR=" + clientAddress);
    envVars.push_back("REMOTE_HOST=" + clientAddress);
    envVars.push_back("PATH_INFO=");
    envVars.push_back("PATH_TRANSLATED=");
    envVars.push_back("REQUEST_URI=" + request.getUri());
    envVars.push_back("DOCUMENT_ROOT=" + documentRoot);
    envVars.push_back("REDIRECT_STATUS=CGI");
    
    if (request.getMethod() == "POST") {
        std::string contentType = request.getHeader("Content-Type");
        if (contentType.empty()) {
            contentType = "application/x-www-form-urlencoded";
        }
        envVars.push_back("CONTENT_TYPE=" + contentType);
        
        std::ostringstream contentLength;
        contentLength << postData.length();
        envVars.push_back("CONTENT_LENGTH=" + contentLength.str());
    }
    
    // Pass all HTTP headers as HTTP_* environment variables (CGI spec requirement)
    const std::map<std::string, std::string>& headers = request.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); 
         it != headers.end(); ++it) {
        std::string headerName = it->first;
        std::string headerValue = it->second;
        
        // Convert header name to CGI format: "User-Agent" -> "HTTP_USER_AGENT"
        for (size_t i = 0; i < headerName.length(); ++i) {
            if (headerName[i] == '-') {
                headerName[i] = '_';
            } else {
                headerName[i] = std::toupper(headerName[i]);
            }
        }
        
        // Skip Content-Type and Content-Length (already handled above)
        if (headerName != "CONTENT_TYPE" && headerName != "CONTENT_LENGTH") {
            envVars.push_back("HTTP_" + headerName + "=" + headerValue);
        }
    }
    
    char **env = new char*[envVars.size() + 1];
    for (size_t i = 0; i < envVars.size(); ++i) {
        size_t len = envVars[i].length();
        env[i] = new char[len + 1];
        for (size_t j = 0; j < len; ++j) {
            env[i][j] = envVars[i][j];
        }
        env[i][len] = '\0';
    }
    env[envVars.size()] = NULL;
    
    return env;
}

void CgiOperation::freeCgiEnvironment(char** env)
{
    if (!env) return;
    
    for (int i = 0; env[i] != NULL; ++i) {
        delete[] env[i];
    }
    delete[] env;
}

bool CgiOperation::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "CgiOperation: fcntl F_GETFL failed: " << strerror(errno) << std::endl;
        return false;
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "CgiOperation: fcntl F_SETFL failed: " << strerror(errno) << std::endl;
        return false;
    }
    
    return true;
}

void CgiOperation::readFromProcess()
{
    ssize_t bytesRead = read(outputFd, readBuffer, BUFFER_SIZE - 1);
    
    if (bytesRead > 0) {
        readBuffer[bytesRead] = '\0';
        result += std::string(readBuffer, bytesRead);
    } else if (bytesRead == 0) {
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        error = true;
        errorMessage = "Error reading from CGI process";
    }
}

void CgiOperation::writePostData()
{
    if (postData.empty() && inputFd < 0) return;
    
    if (!postData.empty() && inputFd >= 0) {
        std::cout << "CgiOperation: Attempting to write " << postData.length() << " bytes to fd " << inputFd << std::endl;
        ssize_t bytesWritten = write(inputFd, postData.c_str(), postData.length());
        
        if (bytesWritten > 0) {
            std::cout << "CgiOperation: Successfully wrote " << bytesWritten << " bytes" << std::endl;
            postData.erase(0, bytesWritten);
            
            // If all data written, close input fd immediately to signal EOF to CGI
            if (postData.empty() && inputFd >= 0) {
                std::cout << "CgiOperation: All POST data written, closing input fd" << std::endl;
                close(inputFd);
                inputFd = -1;
            }
        } else if (bytesWritten == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "CgiOperation: Write error: " << strerror(errno) << std::endl;
            close(inputFd);
            inputFd = -1;
        } else if (bytesWritten == -1) {
            std::cout << "CgiOperation: Write would block (EAGAIN), will retry" << std::endl;
        }
    }
}

bool CgiOperation::checkProcessStatus()
{
    if (childPid <= 0) return true;
    
    int status;
    pid_t result = waitpid(childPid, &status, WNOHANG);
    
    if (result == childPid) {
        if (WIFEXITED(status)) {
            int exitCode = WEXITSTATUS(status);
            
            if (exitCode != 0) {
                error = true;
                errorMessage = "CGI process exited with non-zero status";
            }
        } else if (WIFSIGNALED(status)) {
            int signal = WTERMSIG(status);
            std::cout << "CgiOperation: CGI process killed by signal " << signal << std::endl;
            error = true;
            errorMessage = "CGI process was terminated by signal";
        }
        
        childPid = -1;
        return true;
    } else if (result == -1) {
        std::cerr << "CgiOperation: waitpid error: " << strerror(errno) << std::endl;
        error = true;
        errorMessage = "Error waiting for CGI process";
        return true;
    }
    
    return false;
}

void CgiOperation::forceCompletion()
{
    completed = true;
    
    if (childPid > 0) {
        int status;
        pid_t result = waitpid(childPid, &status, WNOHANG);
        if (result == childPid) {
            if (WIFEXITED(status)) {
                int exitCode = WEXITSTATUS(status);
                if (exitCode != 0) {
                    error = true;
                }
            } else if (WIFSIGNALED(status)) {
                error = true;
            }
            childPid = -1;
        }
    }
}