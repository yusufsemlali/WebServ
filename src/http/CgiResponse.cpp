#include <iostream>

char **Env()
{
    char *env[0] = strdup("MY_ENV_VAR=value");
    return env;
}

// pathCgi = /bin/....
void ExecuteCgi (const std::string& scriptName, std::string pathCgi)
{

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
        return;
    }

    if (pid == 0) 
    {
        // Child process
        // Set up the environment variables
        char **sEnv = Env();
        char **args = new char*[3];
        args[0] = strdup(pathCgi.c_str());
        args[1] = strdup(scriptName.c_str());
        args[2] = nullptr;

        // Redirect stdout and stderr to the pipe
        dup2(pipefd[1], 1); // =

        // dup2(pipefd[1], 2);
        close(pipefd[1]);
        close(pipefd[0]);

        execve(pathCgi, args, sEnv);
        std::cerr << "Failed to execute CGI script" << std::endl;
        exit(1);
    }

    // Parent process
    // Wait for the child process to complete
    int status;
    waitpid(pid, &status, 0);
    std::string body;
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

    if (WIFEXITED(status)) 
    {
        std::cout << "CGI script executed with exit code " << WEXITSTATUS(status) << std::endl;
    } else 
    {
        std::cerr << "CGI script terminated abnormally" << std::endl;
    }
}
