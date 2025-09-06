// #include "CgiHandler.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

//every fd action should be executed by epoll
//

// char **createCgiEnv(const std::string& scriptName, const std::string& /* pathCgi */, 
//                     const std::string& requestMethod = "GET", 
//                     const std::string& queryString = "",
//                     const std::string& serverName = "WebServ",
//                     const std::string& serverPort = "8080",
//                     const std::string& remoteAddr = "127.0.0.1")
// {
//     std::vector<std::string> envVars;
    
//     // Static CGI environment variables
//     envVars.push_back("GATEWAY_INTERFACE=CGI/1.1");
//     envVars.push_back("SERVER_SOFTWARE=WebServ/1.0");
//     envVars.push_back("SERVER_PROTOCOL=HTTP/1.1");
//     envVars.push_back("SERVER_NAME=" + serverName);
//     envVars.push_back("SERVER_PORT=" + serverPort);
//     envVars.push_back("REQUEST_METHOD=" + requestMethod);
//     envVars.push_back("SCRIPT_NAME=" + scriptName);
//     envVars.push_back("QUERY_STRING=" + queryString);
//     envVars.push_back("CONTENT_TYPE=text/html");
//     envVars.push_back("REMOTE_ADDR=" + remoteAddr);
//     envVars.push_back("REMOTE_HOST=localhost");
//     envVars.push_back("PATH_INFO=");
//     envVars.push_back("PATH_TRANSLATED=");
    
//     // Convert to char** format
//     char **env = new char*[envVars.size() + 1];
//     for (size_t i = 0; i < envVars.size(); ++i) {
//         env[i] = strdup(envVars[i].c_str());
//     }
//     env[envVars.size()] = NULL;
    
//     return env;
// }

// void freeCgiEnv(char **env)
// {
//     if (!env) return;
    
//     for (int i = 0; env[i] != NULL; ++i) {
//         free(env[i]);
//     }
//     delete[] env;
// }

// char **Env()
// {
//     char **env = new char*[2];
//     env[0] = strdup("MY_ENV_VAR=value");
//     env[1] = NULL;
//     return env;
// }

// pathCgi = /bin/....
void ExecuteCgi (const std::string& scriptName, std::string pathCgi)
{
    // check access if file exists and is executable
    //creating a pipe
    int pipefd[2];
    if (pipe(pipefd) == -1) 
    {
        std::cerr << "Failed to create pipe" << std::endl;
        return;
    }

    // Create a child process to execute the CGI script
    pid_t pid = fork();
    if (pid == -1) 
    {
        std::cerr << "Failed to fork process" << std::endl;
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (pid == 0) 
    {
        // Child process
        // Set up the environment variables with static CGI variables
        // char **sEnv = createCgiEnv(scriptName, pathCgi);
        char **args = new char*[3];
        args[0] = strdup(pathCgi.c_str());
        args[1] = strdup(scriptName.c_str());
        args[2] = NULL;

        // Redirect stdout and stderr to the pipe
        dup2(pipefd[1], 1); // =

        // dup2(pipefd[1], 2);
        close(pipefd[1]);
        close(pipefd[0]);

        execve(pathCgi.c_str(), args, NULL);
        
        // Cleanup in case execve fails
        // freeCgiEnv(sEnv);
        free(args[0]);
        free(args[1]);
        delete[] args;
        
        std::cerr << "Failed to execute CGI script" << std::endl;
        exit(1);
    }

    // Parent process
    close(pipefd[1]); // Close write end in parent
    
    std::string body;
    char buffer[4096];
    while (true)
    {
        ssize_t bytesRead = read(pipefd[0], buffer, sizeof(buffer));
        if (bytesRead > 0)
        {
           body.append(buffer, bytesRead);
        }
        else
        {
            break;
        }
    }
    close(pipefd[0]);
    std::cout << "CGI Script Output:\n" << body << std::endl;
    // Wait for the child process to complete
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) 
    {
        std::cout << "CGI script executed with exit code " << WEXITSTATUS(status) << std::endl;
    } else 
    {
        std::cerr << "CGI script terminated abnormally" << std::endl;
    }
}

// // Constructor and destructor implementations
// CgiHandler::CgiHandler() : response(*(new HttpResponse())) 
// {
//     // Default constructor - creates a new HttpResponse
//     // Note: This creates a memory leak. Better to use the parameterized constructor.
// }

// CgiHandler::~CgiHandler() 
// {
//     // Destructor
// }

// void CgiHandler::ExecuteCgi (const std::string& scriptName, std::string pathCgi)
int main ()
{
    ExecuteCgi("test.py", "/usr/bin/python3");
    return 0;
}