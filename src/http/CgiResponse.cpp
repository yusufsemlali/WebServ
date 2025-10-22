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
    
    // Static CGI environment variables
    envVars.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envVars.push_back("SERVER_SOFTWARE=WebServ/1.0");
    envVars.push_back("SERVER_PROTOCOL=HTTP/1.1");
    envVars.push_back("SERVER_NAME=" + serverName);
    envVars.push_back("SERVER_PORT=" + serverPort);
    envVars.push_back("REQUEST_METHOD=" + requestMethod);
    envVars.push_back("SCRIPT_NAME=" + scriptUri);
    envVars.push_back("SCRIPT_FILENAME=" + scriptName);  // Critical for PHP CGI!
    envVars.push_back("QUERY_STRING=" + queryString);
    envVars.push_back("REMOTE_ADDR=" + remoteAddr);
    envVars.push_back("REMOTE_HOST=localhost");
    envVars.push_back("PATH_INFO=");
    envVars.push_back("PATH_TRANSLATED=");
    
    // Additional PHP CGI specific variables
    envVars.push_back("REQUEST_URI=" + scriptUri);
    envVars.push_back("DOCUMENT_ROOT=" + documentRoot);  // Now configurable
    envVars.push_back("HTTP_HOST=" + serverName + ":" + serverPort);
    envVars.push_back("REDIRECT_STATUS=200");  // Critical for PHP CGI security
    
    // Add content type and length for POST requests
    if (!contentType.empty()) {
        envVars.push_back("CONTENT_TYPE=" + contentType);
    } else {
        envVars.push_back("CONTENT_TYPE=text/html");
    }
    
    if (!contentLength.empty()) {
        envVars.push_back("CONTENT_LENGTH=" + contentLength);
    }
    
    // Convert to char** format
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

// pathCgi = /bin/....
void CgiHandler::ExecuteCgi (const std::string& scriptName, std::string pathCgi, const HttpRequest& request)
{
    std::cout << "CGI: Executing script: " << scriptName << " with interpreter: " << pathCgi << std::endl;
    
    // Check if script file exists and is readable
    if (access(scriptName.c_str(), R_OK) != 0) {
        std::cerr << "CGI: Script file not found or not readable: " << scriptName << std::endl;
        response.setStatus(404, "Not Found");
        response.setBody("CGI script not found");
        response.setHeader("Content-Type", "text/plain");
        return;
    }
    
    // Check if interpreter exists and is executable
    if (access(pathCgi.c_str(), X_OK) != 0) {
        std::cerr << "CGI: Interpreter not found or not executable: " << pathCgi << std::endl;
        response.setStatus(500, "Internal Server Error");
        response.setBody("CGI interpreter not available");
        response.setHeader("Content-Type", "text/plain");
        return;
    }
    
    // Create pipes for communication
    int pipefd[2];  // For stdout from child
    int stdinPipe[2];  // For stdin to child (POST data)
    
    if (pipe(pipefd) == -1 || pipe(stdinPipe) == -1) 
    {
        std::cerr << "CGI: Failed to create pipe" << std::endl;
        response.setStatus(500, "Internal Server Error");
        response.setBody("CGI execution failed");
        response.setHeader("Content-Type", "text/plain");
        return;
    }

    // Create a child process to execute the CGI script
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
        // Child process
        // Set up the environment variables with request information
        std::string contentLength = "";
        if (request.getMethod() == "POST") {
            // Use actual body size instead of the parsed Content-Length header
            std::ostringstream oss;
            oss << request.getBody().length();
            contentLength = oss.str();
        }
        
        // Use script path as-is for SCRIPT_FILENAME (42 compliant)
        std::string absoluteScriptName = scriptName;
        
        char **sEnv = createCgiEnv(absoluteScriptName, request.getUri(), pathCgi, 
                                   request.getMethod(), 
                                   request.getQuery(),
                                   request.getContentType(),
                                   contentLength);
        
        // Debug: Print environment variables being set
        std::cout << "CGI: Environment variables being set:" << std::endl;
        for (int i = 0; sEnv[i] != NULL; ++i) {
            std::cout << "  " << sEnv[i] << std::endl;
        }
        
        char **args = new char*[3];
        args[0] = strdup(pathCgi.c_str());
        args[1] = strdup(scriptName.c_str());
        args[2] = NULL;

        // Redirect stdout to the pipe
        dup2(pipefd[1], STDOUT_FILENO);
        // Redirect stdin for POST data
        dup2(stdinPipe[0], STDIN_FILENO);
        
        // Close all pipe ends in child
        close(pipefd[1]);
        close(pipefd[0]);
        close(stdinPipe[0]);
        close(stdinPipe[1]);

        execve(pathCgi.c_str(), args, sEnv);
        
        // Cleanup in case execve fails
        freeCgiEnv(sEnv);
        free(args[0]);
        free(args[1]);
        delete[] args;
        
        std::cerr << "CGI: Failed to execute script" << std::endl;
        exit(1);
    }

    // Parent process
    close(pipefd[1]); // Close write end of stdout pipe
    close(stdinPipe[0]); // Close read end of stdin pipe
    
    // Send POST data to child if this is a POST request
    if (request.getMethod() == "POST" && !request.getBody().empty()) {
        write(stdinPipe[1], request.getBody().c_str(), request.getBody().length());
    }
    close(stdinPipe[1]); // Close write end of stdin pipe
    
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
            break; // End of file
        }
        else
        {
            std::cerr << "CGI: Error reading from pipe" << std::endl;
            break;
        }
    }
    close(pipefd[0]);

    // Wait for the child process to complete
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) 
    {
        int exitCode = WEXITSTATUS(status);
        std::cout << "CGI: Script executed with exit code " << exitCode << std::endl;
        
        if (exitCode == 0) {
            // Success - parse CGI output
            parseCgiOutput(body);
        } else {
            // Script failed
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

// Constructor and destructor implementations  
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
    
    // Find the separation between headers and body (double newline)
    size_t headerEnd = output.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = output.find("\n\n");
    }
    
    if (headerEnd != std::string::npos) {
        // Parse headers
        std::string headers = output.substr(0, headerEnd);
        std::string body = output.substr(headerEnd + (output.find("\r\n\r\n") != std::string::npos ? 4 : 2));
        
        // Parse individual headers
        std::istringstream headerStream(headers);
        std::string line;
        bool statusSet = false;
        
        while (std::getline(headerStream, line)) {
            // Remove carriage return if present
            if (!line.empty() && line[line.length() - 1] == '\r') {
                line = line.substr(0, line.length() - 1);
            }
            
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string headerName = line.substr(0, colonPos);
                std::string headerValue = line.substr(colonPos + 1);
                
                // Trim whitespace
                while (!headerValue.empty() && headerValue[0] == ' ') {
                    headerValue = headerValue.substr(1);
                }
                
                if (headerName == "Status" && !statusSet) {
                    // Parse status code
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
        
        // Set default status if not provided
        if (!statusSet) {
            response.setStatus(200, "OK");
        }
        
        response.setBody(body);
        std::cout << "CGI: Successfully parsed headers and body" << std::endl;
    } else {
        // No headers found, treat entire output as body
        response.setStatus(200, "OK");
        response.setBody(output);
        response.setHeader("Content-Type", "text/html");
        std::cout << "CGI: No headers found, treating as plain body" << std::endl;
    }
}
