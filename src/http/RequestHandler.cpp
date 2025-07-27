#include "RequestHandler.hpp"

#include <iostream>

RequestHandler::RequestHandler(const Config &config)
    : config(config)
{
}

RequestHandler::~RequestHandler()
{
    // TODO: Cleanup request handler
}

void RequestHandler::handleRequest(const HttpRequest &request, HttpResponse &response)
{
    std::string method = request.getMethod();
    std::string uri = request.getUri();

    std::cout << "Processing " << method << " request for " << uri << std::endl;

    // 1. Validate request
    if (!isValidRequest(request))
    {
        setErrorResponse(400, response, "Bad Request");
        return;
    }

    const Config::ServerConfig &serverConfig = findServerConfig(request);

    const Config::LocationConfig &locationConfig = findLocationConfig(serverConfig, uri);

    if (!isMethodAllowed(method, locationConfig))
    {
        serveErrorPage(405, response, serverConfig);
        return;
    }

    if (method == "GET")
    {
        processGetRequest(request, response, serverConfig, locationConfig);
    }
    else if (method == "POST")
    {
        processPostRequest(request, response, serverConfig, locationConfig);
    }
    else if (method == "DELETE")
    {
        processDeleteRequest(request, response, serverConfig, locationConfig);
    }
    else
    {
        serveErrorPage(501, response, serverConfig);
    }
}

const Config::ServerConfig &RequestHandler::findServerConfig(const HttpRequest &request) const
{
    std::string hostHeader = request.getHeader("Host");
    std::string hostname = hostHeader;

    size_t colonPos = hostname.find(':');
    if (colonPos != std::string::npos)
    {
        hostname = hostname.substr(0, colonPos);
    }

    const std::vector<Config::ServerConfig> &servers = config.servers;
    for (size_t i = 0; i < servers.size(); ++i)
    {
        const std::vector<std::string> &serverNames = servers[i].serverNames;
        for (size_t j = 0; j < serverNames.size(); ++j)
        {
            if (serverNames[j] == hostname)
            {
                return servers[i];
            }
        }
    }

    return servers[0];
}

const Config::LocationConfig &RequestHandler::findLocationConfig(const Config::ServerConfig &server, const std::string &uri) const
{
    const std::vector<Config::LocationConfig> &locations = server.locations;
    const Config::LocationConfig *bestMatch = NULL;
    size_t bestMatchLength = 0;

    for (size_t i = 0; i < locations.size(); ++i)
    {
        const std::string &locationPath = locations[i].path;
        if (uri.find(locationPath) == 0 && locationPath.length() > bestMatchLength)
        {
            bestMatch = &locations[i];
            bestMatchLength = locationPath.length();
        }
    }

    if (bestMatch)
    {
        return *bestMatch;
    }

    return locations[0];
}

void RequestHandler::processGetRequest(const HttpRequest &request, HttpResponse &response, const Config::ServerConfig &server, const Config::LocationConfig &location)
{
    (void)request;
    (void)response;
    (void)server;
    (void)location;
    // TODO: Handle GET request
    // 1. Resolve file path
    // 2. Check if file exists
    // 3. Serve file or directory listing
}

void RequestHandler::processPostRequest(const HttpRequest &request, HttpResponse &response, const Config::ServerConfig &server, const Config::LocationConfig &location)
{
    (void)request;
    (void)response;
    (void)server;
    (void)location;
    // TODO: Handle POST request

    // 1. Check content length
    // 2. Process request body
    // 3. Execute CGI or handle upload
}

void RequestHandler::processDeleteRequest(const HttpRequest &request, HttpResponse &response, const Config::ServerConfig &server, const Config::LocationConfig &location)
{
    (void)request;
    (void)response;
    (void)server;
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
    // If no methods are specified, allow all common methods
    if (location.allowedMethods.empty())
    {
        return (method == "GET" || method == "POST" || method == "DELETE" || method == "HEAD");
    }

    return location.allowedMethods.find(method) != location.allowedMethods.end();
}

bool RequestHandler::isValidRequest(const HttpRequest &request) const
{
    if (!request.isComplete())
    {
        std::cerr << "Request parsing incomplete" << std::endl;
        return false;
    }

    if (!request.isValidMethod())
    {
        std::cerr << "Invalid HTTP method: " << request.getMethod() << std::endl;
        return false;
    }

    if (!request.isValidVersion())
    {
        std::cerr << "Invalid HTTP version: " << request.getVersion() << std::endl;
        return false;
    }

    if (!request.isValidUri())
    {
        std::cerr << "Invalid URI: " << request.getUri() << std::endl;
        return false;
    }

    // Additional business logic validations can go here
    // e.g., Content-Length validation for POST requests
    if (request.getMethod() == "POST")
    {
        std::string contentLength = request.getHeader("Content-Length");
        if (contentLength.empty())
        {
            std::cerr << "POST request missing Content-Length header" << std::endl;
            return false;
        }
    }

    return true;
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
