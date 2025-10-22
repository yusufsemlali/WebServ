#include "CgiHandler.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

//every fd action should be executed by epoll
//

char **createCgiEnv(const std::string& scriptName, const std::string& scriptUri, const std::string& /* pathCgi */, 
                    const std::string& requestMethod = "GET", 
                    const std::string& queryString = "",
                    const std::string& contentType = "",
                    const std::string& contentLength = "",
                    const std::string& serverName = "WebServ",
                    const std::string& serverPort = "8080",
                    const std::string& remoteAddr = "127.0.0.1",
                    const std::string& documentRoot = "./www")
{
    std::vector<std::string> envVars;
    
    envVars.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envVars.push_back("SERVER_SOFTWARE=WebServ/1.0");
    envVars.push_back("SERVER_PROTOCOL=HTTP/1.1");
    envVars.push_back("SERVER_NAME=" + serverName);
    envVars.push_back("SERVER_PORT=" + serverPort);
    envVars.push_back("REQUEST_METHOD=" + requestMethod);
    envVars.push_back("SCRIPT_NAME=" + scriptUri);
    envVars.push_back("SCRIPT_FILENAME=" + scriptName);
    envVars.push_back("QUERY_STRING=" + queryString);
    envVars.push_back("REMOTE_ADDR=" + remoteAddr);
    envVars.push_back("REMOTE_HOST=localhost");
    envVars.push_back("PATH_INFO=");
    envVars.push_back("PATH_TRANSLATED=");
    
    envVars.push_back("REQUEST_URI=" + scriptUri);
    envVars.push_back("DOCUMENT_ROOT=" + documentRoot);
    envVars.push_back("HTTP_HOST=" + serverName + ":" + serverPort);
    envVars.push_back("REDIRECT_STATUS=200");
    
    if (!contentType.empty()) {
        envVars.push_back("CONTENT_TYPE=" + contentType);
    } else {
        envVars.push_back("CONTENT_TYPE=text/html");
    }
    
    if (!contentLength.empty()) {
        envVars.push_back("CONTENT_LENGTH=" + contentLength);
    }
    
    char **env = new char*[envVars.size() + 1];
    for (size_t i = 0; i < envVars.size(); ++i) {
        env[i] = strdup(envVars[i].c_str());
    }
    env[envVars.size()] = NULL;
    
    return env;
}

void freeCgiEnv(char **env)
{
    if (!env) return;
    
    for (int i = 0; env[i] != NULL; ++i) {
        free(env[i]);
    }
    delete[] env;
}

void CgiHandler::ExecuteCgi (const std::string& scriptName, std::string pathCgi, const HttpRequest& request)
{
    std::cout << "CGI: Executing script: " << scriptName << " with interpreter: " << pathCgi << std::endl;
    
    if (access(scriptName.c_str(), R_OK) != 0) {
        std::cerr << "CGI: Script file not found or not readable: " << scriptName << std::endl;
        response.setStatus(404, "Not Found");
        response.setBody("CGI script not found");
        response.setHeader("Content-Type", "text/plain");
        return;
    }
    
    if (access(pathCgi.c_str(), X_OK) != 0) {
        std::cerr << "CGI: Interpreter not found or not executable: " << pathCgi << std::endl;
        response.setStatus(500, "Internal Server Error");
        response.setBody("CGI interpreter not available");
        response.setHeader("Content-Type", "text/plain");
        return;
    }
    
    int pipefd[2];
    int stdinPipe[2];
    
    if (pipe(pipefd) == -1 || pipe(stdinPipe) == -1) 
    {
        std::cerr << "CGI: Failed to create pipe" << std::endl;
        response.setStatus(500, "Internal Server Error");
        response.setBody("CGI execution failed");
        response.setHeader("Content-Type", "text/plain");
        return;
    }

    pid_t pid = fork();
    if (pid == -1) 
    {
        std::cerr << "CGI: Failed to fork process" << std::endl;
        close(pipefd[0]);
        close(pipefd[1]);
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        response.setStatus(500, "Internal Server Error");
        response.setBody("CGI execution failed");
        response.setHeader("Content-Type", "text/plain");
        return;
    }

    if (pid == 0) 
    {
        std::string contentLength = "";
        if (request.getMethod() == "POST") {
            std::ostringstream oss;
            oss << request.getBody().length();
            contentLength = oss.str();
            std::cout << "CGI: POST detected, using actual body length: " << contentLength << std::endl;
            std::cout << "CGI: POST body size: " << request.getBody().length() << std::endl;
        }
        
        std::string absoluteScriptName = scriptName;
        
        char **sEnv = createCgiEnv(absoluteScriptName, request.getUri(), pathCgi, 
                                   request.getMethod(), 
                                   request.getQuery(),
                                   request.getContentType(),
                                   contentLength);
        
        std::cout << "CGI: Environment variables being set:" << std::endl;
        for (int i = 0; sEnv[i] != NULL; ++i) {
            std::cout << "  " << sEnv[i] << std::endl;
        }
        
        char **args = new char*[3];
        args[0] = strdup(pathCgi.c_str());
        args[1] = strdup(scriptName.c_str());
        args[2] = NULL;

        dup2(pipefd[1], STDOUT_FILENO);
        dup2(stdinPipe[0], STDIN_FILENO);
        
        close(pipefd[1]);
        close(pipefd[0]);
        close(stdinPipe[0]);
        close(stdinPipe[1]);

        execve(pathCgi.c_str(), args, sEnv);
        
        freeCgiEnv(sEnv);
        free(args[0]);
        free(args[1]);
        delete[] args;
        
        std::cerr << "CGI: Failed to execute script" << std::endl;
        exit(1);
    }

    close(pipefd[1]);
    close(stdinPipe[0]);
    
    if (request.getMethod() == "POST" && !request.getBody().empty()) {
        write(stdinPipe[1], request.getBody().c_str(), request.getBody().length());
    }
    close(stdinPipe[1]);
    
    std::string body;
    char buffer[4096];
    while (true)
    {
        ssize_t bytesRead = read(pipefd[0], buffer, sizeof(buffer));
        if (bytesRead > 0)
        {
           body.append(buffer, bytesRead);
        }
        else if (bytesRead == 0)
        {
            break;
        }
        else
        {
            std::cerr << "CGI: Error reading from pipe" << std::endl;
            break;
        }
    }
    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) 
    {
        int exitCode = WEXITSTATUS(status);
        std::cout << "CGI: Script executed with exit code " << exitCode << std::endl;
        
        if (exitCode == 0) {
            parseCgiOutput(body);
        } else {
            response.setStatus(500, "Internal Server Error");
            response.setBody("CGI script execution failed");
            response.setHeader("Content-Type", "text/plain");
        }
    } else 
    {
        std::cerr << "CGI: Script terminated abnormally" << std::endl;
        response.setStatus(500, "Internal Server Error");
        response.setBody("CGI script terminated abnormally");
        response.setHeader("Content-Type", "text/plain");
    }
}

CgiHandler::CgiHandler() : response(*(new HttpResponse())) 
{
    // FIXME: This creates a memory leak - HttpResponse should be managed properly
    // Better to require HttpResponse& in constructor or use smart pointers
}

CgiHandler::~CgiHandler() 
{
    // FIXME: Cannot safely delete response here as it might be stack-allocated
    // This is why the parameterized constructor CgiHandler(HttpResponse &res) is preferred
}

void CgiHandler::parseCgiOutput(const std::string& output)
{
    std::cout << "CGI: Parsing output (" << output.length() << " bytes)" << std::endl;
    
    size_t headerEnd = output.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = output.find("\n\n");
    }
    
    if (headerEnd != std::string::npos) {
        std::string headers = output.substr(0, headerEnd);
        std::string body = output.substr(headerEnd + (output.find("\r\n\r\n") != std::string::npos ? 4 : 2));
        
        std::istringstream headerStream(headers);
        std::string line;
        bool statusSet = false;
        
        while (std::getline(headerStream, line)) {
            if (!line.empty() && line[line.length() - 1] == '\r') {
                line = line.substr(0, line.length() - 1);
            }
            
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string headerName = line.substr(0, colonPos);
                std::string headerValue = line.substr(colonPos + 1);
                
                while (!headerValue.empty() && headerValue[0] == ' ') {
                    headerValue = headerValue.substr(1);
                }
                
                if (headerName == "Status" && !statusSet) {
                    std::istringstream statusStream(headerValue);
                    int statusCode;
                    std::string statusMessage;
                    statusStream >> statusCode;
                    std::getline(statusStream, statusMessage);
                    if (!statusMessage.empty() && statusMessage[0] == ' ') {
                        statusMessage = statusMessage.substr(1);
                    }
                    response.setStatus(statusCode, statusMessage);
                    statusSet = true;
                } else {
                    response.setHeader(headerName, headerValue);
                }
            }
        }
        
        if (!statusSet) {
            response.setStatus(200, "OK");
        }
        
        response.setBody(body);
        std::cout << "CGI: Successfully parsed headers and body" << std::endl;
    } else {
        response.setStatus(200, "OK");
        response.setBody(output);
        response.setHeader("Content-Type", "text/html");
        std::cout << "CGI: No headers found, treating as plain body" << std::endl;
    }
}
