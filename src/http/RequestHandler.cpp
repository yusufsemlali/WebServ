#include "RequestHandler.hpp"
#include <iostream>

RequestHandler::RequestHandler(const Config &config) : config(config)
{
    // TODO: Initialize request handler
}

RequestHandler::~RequestHandler()
{
    // TODO: Cleanup request handler
}

void RequestHandler::handleRequest(const HttpRequest &request, HttpResponse &response)
{
    (void)request;
    (void)response;
    // TODO: Main request handling logic
    // 1. Validate request
    // 2. Find server config
    // 3. Find location config
    // 4. Check method permissions
    // 5. Route to appropriate handler
}

const Config::ServerConfig* RequestHandler::findServerConfig(const HttpRequest &request) const
{
    (void)request;
    // TODO: Find matching server config based on Host header and server_name
    if (!config.servers.empty())
        return &config.servers[0]; // Default to first server for now
    return NULL;
}

const Config::LocationConfig* RequestHandler::findLocationConfig(const Config::ServerConfig &server, const std::string &uri) const
{
    (void)server;
    (void)uri;
    // TODO: Find best matching location config for URI
    return NULL;
}

void RequestHandler::processGetRequest(const HttpRequest &request, HttpResponse &response, const Config::LocationConfig &location)
{
    (void)request;
    (void)response;
    (void)location;
    // TODO: Handle GET request
    // 1. Resolve file path
    // 2. Check if file exists
    // 3. Serve file or directory listing
}

void RequestHandler::processPostRequest(const HttpRequest &request, HttpResponse &response, const Config::LocationConfig &location)
{
    (void)request;
    (void)response;
    (void)location;
    // TODO: Handle POST request
    // 1. Check content length
    // 2. Process request body
    // 3. Execute CGI or handle upload
}

void RequestHandler::processDeleteRequest(const HttpRequest &request, HttpResponse &response, const Config::LocationConfig &location)
{
    (void)request;
    (void)response;
    (void)location;
    // TODO: Handle DELETE request
    // 1. Check permissions
    // 2. Delete file/resource
    // 3. Return appropriate response
}

void RequestHandler::serveStaticFile(const std::string &filePath, HttpResponse &response)
{
    (void)filePath;
    (void)response;
    // TODO: Serve static file
    // 1. Check file permissions
    // 2. Set content type
    // 3. Read and set file content
}

void RequestHandler::serveDirectoryListing(const std::string &dirPath, HttpResponse &response)
{
    (void)dirPath;
    (void)response;
    // TODO: Generate directory listing HTML
}

void RequestHandler::serveErrorPage(int errorCode, HttpResponse &response, const Config::ServerConfig &server)
{
    (void)errorCode;
    (void)response;
    (void)server;
    // TODO: Serve custom error page or generate default
}

void RequestHandler::executeCgi(const HttpRequest &request, HttpResponse &response, const Config::LocationConfig &location)
{
    (void)request;
    (void)response;
    (void)location;
    // TODO: Execute CGI script
    // 1. Set environment variables
    // 2. Execute script
    // 3. Parse CGI output
}

bool RequestHandler::isMethodAllowed(const std::string &method, const Config::LocationConfig &location) const
{
    (void)method;
    (void)location;
    // TODO: Check if HTTP method is allowed for this location
    return true; // Default allow all for now
}

bool RequestHandler::isValidRequest(const HttpRequest &request) const
{
    // TODO: Validate HTTP request
    return request.isValidMethod() && request.isValidVersion();
}

std::string RequestHandler::getMimeType(const std::string &filePath) const
{
    (void)filePath;
    // TODO: Determine MIME type based on file extension
    return "text/plain";
}

std::string RequestHandler::resolveFilePath(const std::string &uri, const Config::LocationConfig &location) const
{
    (void)uri;
    (void)location;
    // TODO: Resolve full file path from URI and location config
    return "";
}

void RequestHandler::handleRedirect(const HttpRequest &request, HttpResponse &response, const Config::LocationConfig &location)
{
    (void)request;
    (void)response;
    (void)location;
    // TODO: Handle redirect directive
}

bool RequestHandler::fileExists(const std::string &path) const
{
    (void)path;
    // TODO: Check if file exists
    return false;
}

bool RequestHandler::isDirectory(const std::string &path) const
{
    (void)path;
    // TODO: Check if path is directory
    return false;
}

bool RequestHandler::hasPermission(const std::string &path) const
{
    (void)path;
    // TODO: Check file permissions
    return false;
}

std::string RequestHandler::getFileExtension(const std::string &path) const
{
    // TODO: Extract file extension
    size_t pos = path.find_last_of('.');
    if (pos != std::string::npos)
        return path.substr(pos);
    return "";
}

void RequestHandler::setErrorResponse(int statusCode, HttpResponse &response, const std::string &message)
{
    (void)message;
    response.setStatus(statusCode, message);
    // TODO: Set error page content
}
