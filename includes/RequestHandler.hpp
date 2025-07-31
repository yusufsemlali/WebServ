#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <string>
#include <map>
#include <ctime>
#include "Config.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class RequestHandler
{
public:
    RequestHandler(const Config &config);
    ~RequestHandler();

    void handleRequest(const HttpRequest &request, HttpResponse &response);

private:
    const Config &config;

    // Main request processing methods
    void processGetRequest(const HttpRequest &request, HttpResponse &response, 
                          const Config::ServerConfig &server, const Config::LocationConfig &location);
    void processPostRequest(const HttpRequest &request, HttpResponse &response, 
                           const Config::ServerConfig &server, const Config::LocationConfig &location);
    void processDeleteRequest(const HttpRequest &request, HttpResponse &response, 
                             const Config::ServerConfig &server, const Config::LocationConfig &location);

    // Content serving methods
    void serveStaticFile(const std::string &filePath, HttpResponse &response);
    void serveDirectoryListing(const std::string &dirPath, HttpResponse &response);
    void serveErrorPage(int errorCode, HttpResponse &response, const Config::ServerConfig &server);

    // Special handling methods
    void executeCgi(HttpResponse &response, const Config::LocationConfig &location);
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