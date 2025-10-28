#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <string>
#include <ctime>
#include "Config.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "SocketManager.hpp"

class ClientConnection;

class RequestHandler
{
public:
    RequestHandler(const Config &config, SocketManager &socketManager);
    ~RequestHandler();

    void handleRequest(const HttpRequest &request, HttpResponse &response, ClientConnection* connection = NULL);
    
    void generateErrorPage(int errorCode, HttpResponse &response, int serverFd);
    
    int validateRequestEarly(const HttpRequest &request, int serverFd) const;

private:
    const Config &config;
    SocketManager &socketManager;

    void processGetRequest(const HttpRequest &request, HttpResponse &response, 
                          const Config::ServerConfig &server, const Config::LocationConfig &location,
                          ClientConnection* connection = NULL);
    void processPostRequest(const HttpRequest &request, HttpResponse &response, 
                           const Config::ServerConfig &server, const Config::LocationConfig &location,
                           ClientConnection* connection = NULL);
    void processDeleteRequest(const HttpRequest &request, HttpResponse &response, 
                             const Config::ServerConfig &server, const Config::LocationConfig &location,
                             ClientConnection* connection = NULL);

    void serveStaticFile(const std::string &filePath, HttpResponse &response,
                        const Config::LocationConfig &location, const Config::ServerConfig &server);
    void serveDirectoryListing(const std::string &dirPath, HttpResponse &response);
    void serveErrorPage(int errorCode, HttpResponse &response, const Config::ServerConfig &server);

    void executeCgi(const HttpRequest &request, HttpResponse &response, 
                   const Config::ServerConfig &server, const Config::LocationConfig &location,
                   ClientConnection* connection = NULL);
    void handleRedirect(HttpResponse &response, const Config::LocationConfig &location);
    void handleFileUpload(const HttpRequest &request, HttpResponse &response, 
                         const Config::ServerConfig &server, const Config::LocationConfig &location,
                         ClientConnection* connection = NULL);
    void handleFormData(const HttpRequest &request, HttpResponse &response);

    const Config::ServerConfig &findServerConfig(const HttpRequest &request, int serverFd) const;
    const Config::LocationConfig &findLocationConfig(const Config::ServerConfig &server, const std::string &uri) const;
    
    std::string toLower(const std::string &str) const;
    std::string urlDecode(const std::string &str) const;

    std::string resolveFilePath(const std::string &uri, const Config::LocationConfig &location, 
                               const Config::ServerConfig &server) const;
    bool fileExists(const std::string &path) const;
    bool isDirectory(const std::string &path) const;
    bool hasPermission(const std::string &path) const;
    bool createFile(const std::string &path, const std::string &content) const;

    bool isMethodAllowed(const std::string &method, const Config::LocationConfig &location, 
                        const Config::ServerConfig &server) const;
    bool isValidRequest(const HttpRequest &request) const;
    std::string getMimeType(const std::string &filePath) const;
    void setErrorResponse(int statusCode, HttpResponse &response, const std::string &message);
    std::string getCurrentTimestamp() const;
};

#endif // REQUEST_HANDLER_HPP