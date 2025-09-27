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
                           const HttpRequest& request, const std::string& documentRoot)
    : childPid(-1), outputFd(-1), inputFd(-1), errorFd(-1),
      completed(false), error(false), 
      scriptPath(scriptPath), interpreterPath(interpreterPath), documentRoot(documentRoot)
{
    std::cout << "CgiOperation: Starting CGI for script: " << scriptPath << std::endl;
    
    // Extract POST data if this is a POST request
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
    
    // Read available data from CGI output
    readFromProcess();
    
    // Write POST data if needed
    if (!postData.empty() && inputFd >= 0) {
        writePostData();
    }
    
    // Check if process has finished
    if (checkProcessStatus()) {
        completed = true;
        std::cout << "CgiOperation: CGI process completed" << std::endl;
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
    // Close file descriptors
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
    
    // Wait for child process if still running
    if (childPid > 0) {
        int status;
        if (waitpid(childPid, &status, WNOHANG) == 0) {
            // Process still running, kill it
            kill(childPid, SIGTERM);
            waitpid(childPid, &status, 0);
        }
        childPid = -1;
    }
}

bool CgiOperation::startCgiProcess(const HttpRequest& request)
{
    int pipeStdout[2], pipeStdin[2], pipeStderr[2];
    
    // Create pipes for communication
    if (pipe(pipeStdout) == -1 || pipe(pipeStdin) == -1 || pipe(pipeStderr) == -1) {
        std::cerr << "CgiOperation: Failed to create pipes: " << strerror(errno) << std::endl;
        return false;
    }
    
    // Fork the process
    childPid = fork();
    if (childPid == -1) {
        std::cerr << "CgiOperation: Fork failed: " << strerror(errno) << std::endl;
        close(pipeStdout[0]); close(pipeStdout[1]);
        close(pipeStdin[0]); close(pipeStdin[1]);
        close(pipeStderr[0]); close(pipeStderr[1]);
        return false;
    }
    
    if (childPid == 0) {
        // Child process
        
        // Redirect stdout, stdin, stderr
        dup2(pipeStdout[1], STDOUT_FILENO);
        dup2(pipeStdin[0], STDIN_FILENO);
        dup2(pipeStderr[1], STDERR_FILENO);
        
        // Close unused pipe ends
        close(pipeStdout[0]); close(pipeStdout[1]);
        close(pipeStdin[0]); close(pipeStdin[1]);
        close(pipeStderr[0]); close(pipeStderr[1]);
        
        // Set up environment
        char** env = createCgiEnvironment(request);
        
        // Execute the CGI script
        if (interpreterPath.empty()) {
            // Execute script directly
            char* argv[] = {const_cast<char*>(scriptPath.c_str()), NULL};
            execve(scriptPath.c_str(), argv, env);
        } else {
            // Execute with interpreter
            char* argv[] = {const_cast<char*>(interpreterPath.c_str()), const_cast<char*>(scriptPath.c_str()), NULL};
            execve(interpreterPath.c_str(), argv, env);
        }
        
        // If we get here, exec failed
        freeCgiEnvironment(env);  // Clean up environment on exec failure
        std::cerr << "CgiOperation: exec failed: " << strerror(errno) << std::endl;
        exit(1);
    }
    
    // Parent process
    
    // Close child ends of pipes
    close(pipeStdout[1]);
    close(pipeStdin[0]);
    close(pipeStderr[1]);
    
    // Store our ends of pipes
    outputFd = pipeStdout[0];
    inputFd = pipeStdin[1];
    errorFd = pipeStderr[0];
    
    // Make file descriptors non-blocking
    if (!setNonBlocking(outputFd) || !setNonBlocking(inputFd) || !setNonBlocking(errorFd)) {
        std::cerr << "CgiOperation: Failed to set non-blocking mode" << std::endl;
        return false;
    }
    
    std::cout << "CgiOperation: CGI process started with PID " << childPid << std::endl;
    return true;
}

char** CgiOperation::createCgiEnvironment(const HttpRequest& request)
{
    std::vector<std::string> envVars;
    
    // Basic CGI environment variables
    envVars.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envVars.push_back("SERVER_SOFTWARE=WebServ/1.0");
    envVars.push_back("SERVER_PROTOCOL=HTTP/1.1");
    envVars.push_back("SERVER_NAME=localhost");
    envVars.push_back("SERVER_PORT=8080");
    envVars.push_back("REQUEST_METHOD=" + request.getMethod());
    envVars.push_back("SCRIPT_NAME=" + request.getUri());
    envVars.push_back("SCRIPT_FILENAME=" + scriptPath);
    envVars.push_back("QUERY_STRING=" + request.getQuery());
    envVars.push_back("REMOTE_ADDR=127.0.0.1");
    envVars.push_back("REMOTE_HOST=localhost");
    envVars.push_back("PATH_INFO=");
    envVars.push_back("PATH_TRANSLATED=");
    envVars.push_back("REQUEST_URI=" + request.getUri());
    envVars.push_back("DOCUMENT_ROOT=" + documentRoot);
    envVars.push_back("HTTP_HOST=localhost:8080");
    envVars.push_back("REDIRECT_STATUS=200");
    
    // Content type and length for POST requests
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
    
    // Convert to char** format
    char **env = new char*[envVars.size() + 1];
    for (size_t i = 0; i < envVars.size(); ++i) {
        // Manual string duplication instead of strdup (not allowed)
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
        delete[] env[i];  // Use delete[] instead of free (strdup not allowed)
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
        std::cout << "CgiOperation: Read " << bytesRead << " bytes from CGI" << std::endl;
    } else if (bytesRead == 0) {
        // EOF - CGI closed its stdout
        std::cout << "CgiOperation: CGI closed stdout" << std::endl;
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        // Real error
        std::cerr << "CgiOperation: Read error: " << strerror(errno) << std::endl;
        error = true;
        errorMessage = "Error reading from CGI process";
    }
    // EAGAIN/EWOULDBLOCK means no data available right now, which is normal
}

void CgiOperation::writePostData()
{
    if (postData.empty()) return;
    
    ssize_t bytesWritten = write(inputFd, postData.c_str(), postData.length());
    
    if (bytesWritten > 0) {
        postData.erase(0, bytesWritten);
        std::cout << "CgiOperation: Wrote " << bytesWritten << " bytes to CGI stdin" << std::endl;
        
        if (postData.empty()) {
            // All POST data sent, close stdin
            close(inputFd);
            inputFd = -1;
            std::cout << "CgiOperation: All POST data sent, closed stdin" << std::endl;
        }
    } else if (bytesWritten == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        // Real error
        std::cerr << "CgiOperation: Write error: " << strerror(errno) << std::endl;
        close(inputFd);
        inputFd = -1;
    }
}

bool CgiOperation::checkProcessStatus()
{
    if (childPid <= 0) return true;
    
    int status;
    pid_t result = waitpid(childPid, &status, WNOHANG);
    
    if (result == childPid) {
        // Process has finished
        if (WIFEXITED(status)) {
            int exitCode = WEXITSTATUS(status);
            std::cout << "CgiOperation: CGI process exited with code " << exitCode << std::endl;
            
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
        // Error in waitpid
        std::cerr << "CgiOperation: waitpid error: " << strerror(errno) << std::endl;
        error = true;
        errorMessage = "Error waiting for CGI process";
        return true;
    }
    
    // Process still running
    return false;
}

void CgiOperation::forceCompletion()
{
    std::cout << "CgiOperation: Forcing completion due to EPOLLHUP" << std::endl;
    
    // If we get EPOLLHUP, it means the CGI process closed stdout
    // This typically means the process is done
    completed = true;
    
    // Try to reap the process one more time
    if (childPid > 0) {
        int status;
        pid_t result = waitpid(childPid, &status, WNOHANG);
        if (result == childPid) {
            if (WIFEXITED(status)) {
                int exitCode = WEXITSTATUS(status);
                std::cout << "CgiOperation: Process exited with code " << exitCode << std::endl;
                if (exitCode != 0) {
                    error = true;
                    errorMessage = "CGI process exited with non-zero status";
                }
            } else if (WIFSIGNALED(status)) {
                int signal = WTERMSIG(status);
                std::cout << "CgiOperation: Process killed by signal " << signal << std::endl;
                error = true;
                errorMessage = "CGI process was terminated by signal";
            }
            childPid = -1;
        } else {
            // Process might still be finishing, but stdout is closed
            // We'll mark as complete anyway since we can't read more data
            std::cout << "CgiOperation: Process still running but stdout closed" << std::endl;
        }
    }
}