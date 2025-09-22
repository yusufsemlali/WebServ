#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <string>
#include <map>
#include <ctime>
#include "Config.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

// TEMPORARY: Forward declaration for non-blocking CGI testing
class HttpServer;
class SocketManager;

class RequestHandler
{
public:
    RequestHandler(const Config &config);
    
    // TEMPORARY: Constructor with HttpServer reference for non-blocking CGI testing
    RequestHandler(const Config &config, HttpServer* server);
    
    ~RequestHandler();

    void handleRequest(const HttpRequest &request, HttpResponse &response);
    
    // TEMPORARY: Non-blocking CGI version that needs client_fd
    void handleRequest(const HttpRequest &request, HttpResponse &response, int client_fd);
    
    // Socket-based version that uses server config from accepting socket (source of truth)
    void handleRequest(const HttpRequest &request, HttpResponse &response, int client_fd, const Config::ServerConfig* serverConfig);
    
    // Nginx-style version that handles multiple servers per socket with server_name matching
    void handleRequestNginxStyle(const HttpRequest &request, HttpResponse &response, int client_fd, int serverFd);

private:
    const Config &config;
    
    // TEMPORARY: HttpServer reference for non-blocking CGI testing
    HttpServer* httpServer;

    // Main request processing methods
    void processGetRequest(const HttpRequest &request, HttpResponse &response, 
                          const Config::ServerConfig &server, const Config::LocationConfig &location);
    void processPostRequest(const HttpRequest &request, HttpResponse &response, 
                           const Config::ServerConfig &server, const Config::LocationConfig &location);
    void processDeleteRequest(const HttpRequest &request, HttpResponse &response, 
                             const Config::ServerConfig &server, const Config::LocationConfig &location);

    // TEMPORARY: Non-blocking versions for CGI testing
    void processGetRequestNonBlocking(const HttpRequest &request, HttpResponse &response, 
                                     const Config::ServerConfig &server, const Config::LocationConfig &location, int client_fd);
    void processPostRequestNonBlocking(const HttpRequest &request, HttpResponse &response, 
                                      const Config::ServerConfig &server, const Config::LocationConfig &location, int client_fd);

    // Content serving methods
    void serveStaticFile(const std::string &filePath, HttpResponse &response);
    void serveDirectoryListing(const std::string &dirPath, HttpResponse &response);
    void serveErrorPage(int errorCode, HttpResponse &response, const Config::ServerConfig &server);

    // Special handling methods
    void executeCgi(const HttpRequest &request, HttpResponse &response, 
                   const Config::ServerConfig &server, const Config::LocationConfig &location);
                   
    // TEMPORARY: Non-blocking CGI version for testing
    void executeCgiNonBlocking(const HttpRequest &request, HttpResponse &response, 
                              const Config::ServerConfig &server, const Config::LocationConfig &location,
                              int client_fd);
    void handleRedirect(HttpResponse &response, const Config::LocationConfig &location);
    void handleFileUpload(const HttpRequest &request, HttpResponse &response, 
                         const Config::ServerConfig &server, const Config::LocationConfig &location);
    void handleFormData(const HttpRequest &request, HttpResponse &response);

    // Configuration resolution methods
    const Config::ServerConfig &findServerConfig(const HttpRequest &request) const;
    const Config::LocationConfig &findLocationConfig(const Config::ServerConfig &server, const std::string &uri) const;

    // Path resolution and file operations
    std::string resolveFilePath(const std::string &uri, const Config::LocationConfig &location, 
                               const Config::ServerConfig &server) const;
    bool fileExists(const std::string &path) const;
    bool isDirectory(const std::string &path) const;
    bool hasPermission(const std::string &path) const;
    bool createFile(const std::string &path, const std::string &content) const;

    // Validation methods
    bool isMethodAllowed(const std::string &method, const Config::LocationConfig &location) const;
    bool isValidRequest(const HttpRequest &request) const;

    // Utility methods
    std::string getMimeType(const std::string &filePath) const;
    void setErrorResponse(int statusCode, HttpResponse &response, const std::string &message);
    std::string getCurrentTimestamp() const;
};

#endif // REQUEST_HANDLER_HPP