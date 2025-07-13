#pragma once

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Config.hpp"
#include <string>

class RequestHandler
{
public:
    RequestHandler(const Config &config);
    ~RequestHandler();

    // Main request processing
    void handleRequest(const HttpRequest &request, HttpResponse &response);
    
    // Route matching
    const Config::ServerConfig* findServerConfig(const HttpRequest &request) const;
    const Config::LocationConfig* findLocationConfig(const Config::ServerConfig &server, const std::string &uri) const;
    
    // Request processing
    void processGetRequest(const HttpRequest &request, HttpResponse &response, const Config::LocationConfig &location);
    void processPostRequest(const HttpRequest &request, HttpResponse &response, const Config::LocationConfig &location);
    void processDeleteRequest(const HttpRequest &request, HttpResponse &response, const Config::LocationConfig &location);
    
    // File operations
    void serveStaticFile(const std::string &filePath, HttpResponse &response);
    void serveDirectoryListing(const std::string &dirPath, HttpResponse &response);
    void serveErrorPage(int errorCode, HttpResponse &response, const Config::ServerConfig &server);
    
    // CGI handling
    void executeCgi(const HttpRequest &request, HttpResponse &response, const Config::LocationConfig &location);
    
    // Validation
    bool isMethodAllowed(const std::string &method, const Config::LocationConfig &location) const;
    bool isValidRequest(const HttpRequest &request) const;
    
    // Utility
    std::string getMimeType(const std::string &filePath) const;
    std::string resolveFilePath(const std::string &uri, const Config::LocationConfig &location) const;

private:
    const Config &config;
    
    // Helper methods
    void handleRedirect(const HttpRequest &request, HttpResponse &response, const Config::LocationConfig &location);
    bool fileExists(const std::string &path) const;
    bool isDirectory(const std::string &path) const;
    bool hasPermission(const std::string &path) const;
    std::string getFileExtension(const std::string &path) const;
    
    // Error handling
    void setErrorResponse(int statusCode, HttpResponse &response, const std::string &message = "");
};
