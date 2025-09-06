#include "RequestHandler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <cstdio>
#include"CgiHandler.hpp"

RequestHandler::RequestHandler(const Config &config)
    : config(config)
{
}

RequestHandler::~RequestHandler()
{
    // Cleanup request handler
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

    // 2. Check if method is allowed
    if (!isMethodAllowed(method, locationConfig))
    {
        serveErrorPage(405, response, serverConfig);
        return;
    }

    // 3. Check body size limit for POST requests
    if (method == "POST")
    {
        size_t contentLength = request.getContentLength();
        size_t maxBodySize = locationConfig.clientMaxBodySize > 0 ? 
                           locationConfig.clientMaxBodySize : serverConfig.clientMaxBodySize;
        
        if (contentLength > maxBodySize)
        {
            serveErrorPage(413, response, serverConfig);
            return;
        }
    }

    // 4. Handle redirects
    if (!locationConfig.returnUrl.empty())
    {
        handleRedirect(response, locationConfig);
        return;
    }

    // 5. Process request based on method
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

void RequestHandler::processGetRequest(const HttpRequest &request, HttpResponse &response, 
                                     const Config::ServerConfig &server, const Config::LocationConfig &location)
{
    std::string uri = request.getUri();
    std::string filePath = resolveFilePath(uri, location, server);
    
    std::cout << "GET: Resolved file path: " << filePath << std::endl;

    // Check if file/directory exists
    if (!fileExists(filePath))
    {
        serveErrorPage(404, response, server);
        return;
    }

    // Check if it's a directory
    if (isDirectory(filePath))
    {
        // Try to serve index file
        std::string indexFile = location.index.empty() ? server.index : location.index;
        if (indexFile.empty())
            indexFile = "index.html";
        
        std::string indexPath = filePath;
        if (indexPath[indexPath.length() - 1] != '/')
            indexPath += "/";
        indexPath += indexFile;

        if (fileExists(indexPath) && !isDirectory(indexPath))
        {
            serveStaticFile(indexPath, response);
        }
        else if (location.autoindex || server.autoindex)
        {
            serveDirectoryListing(filePath, response);
        }
        else
        {
            serveErrorPage(403, response, server);
        }
    }
    else
    {
        // Serve static file
        if (hasPermission(filePath))
        {
            serveStaticFile(filePath, response);
        }
        else
        {
            serveErrorPage(403, response, server);
        }
    }
}

void RequestHandler::processPostRequest(const HttpRequest &request, HttpResponse &response, 
                                      const Config::ServerConfig &server, const Config::LocationConfig &location)
{
    std::string uri = request.getUri();
    
    // Check if CGI is configured for this location
    if (!location.cgiPass.empty())
    {
        executeCgi(response, location);
        return;
    }

    // Handle file uploads or form data
    std::string contentType = request.getContentType();
    
    if (contentType.find("multipart/form-data") != std::string::npos)
    {
        handleFileUpload(request, response, server, location);
    }
    else if (contentType == "application/x-www-form-urlencoded")
    {
        handleFormData(request, response);
    }
    else
    {
        // Simple POST - create/update resource
        std::string filePath = resolveFilePath(uri, location, server);
        
        if (createFile(filePath, request.getBody()))
        {
            response.setStatus(201, "Created");
            response.setBody("Resource created successfully");
            response.setHeader("Content-Type", "text/plain");
        }
        else
        {
            serveErrorPage(500, response, server);
        }
    }
}

void RequestHandler::processDeleteRequest(const HttpRequest &request, HttpResponse &response, 
                                        const Config::ServerConfig &server, const Config::LocationConfig &location)
{
    std::string uri = request.getUri();
    std::string filePath = resolveFilePath(uri, location, server);
    
    std::cout << "DELETE: Attempting to delete: " << filePath << std::endl;

    // Check if file exists
    if (!fileExists(filePath))
    {
        serveErrorPage(404, response, server);
        return;
    }

    // Check permissions
    if (!hasPermission(filePath))
    {
        serveErrorPage(403, response, server);
        return;
    }

    // Don't allow deleting directories for safety
    if (isDirectory(filePath))
    {
        serveErrorPage(403, response, server);
        return;
    }

    // Attempt to delete the file
    if (std::remove(filePath.c_str()) == 0)
    {
        response.setStatus(204, "No Content");
        response.setBody("");
    }
    else
    {
        serveErrorPage(500, response, server);
    }
}

void RequestHandler::serveStaticFile(const std::string &filePath, HttpResponse &response)
{
    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        response.setStatus(404, "Not Found");
        return;
    }

    // Read file content
    std::ostringstream oss;
    oss << file.rdbuf();
    file.close();

    // Set response
    response.setStatus(200, "OK");
    response.setBody(oss.str());
    response.setHeader("Content-Type", getMimeType(filePath));
    
    std::cout << "Served static file: " << filePath << " (" << oss.str().length() << " bytes)" << std::endl;
}

void RequestHandler::serveDirectoryListing(const std::string &dirPath, HttpResponse &response)
{
    DIR* dir = opendir(dirPath.c_str());
    if (!dir)
    {
        response.setStatus(403, "Forbidden");
        return;
    }

    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html>\n<head>\n";
    html << "<title>Index of " << dirPath << "</title>\n";
    html << "<style>\n";
    html << "body { font-family: Arial, sans-serif; margin: 40px; }\n";
    html << "h1 { color: #333; }\n";
    html << "table { border-collapse: collapse; width: 100%; }\n";
    html << "th, td { text-align: left; padding: 8px; border-bottom: 1px solid #ddd; }\n";
    html << "th { background-color: #f2f2f2; }\n";
    html << "a { text-decoration: none; color: #0066cc; }\n";
    html << "a:hover { text-decoration: underline; }\n";
    html << "</style>\n";
    html << "</head>\n<body>\n";
    html << "<h1>Index of " << dirPath << "</h1>\n";
    html << "<table>\n";
    html << "<tr><th>Name</th><th>Size</th><th>Type</th></tr>\n";

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || 
            (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
            continue; // Skip . and ..

        std::string fullPath = dirPath;
        if (fullPath[fullPath.length() - 1] != '/')
            fullPath += "/";
        fullPath += entry->d_name;

        struct stat fileStat;
        std::string fileType = "File";
        std::string fileSize = "-";
        
        if (stat(fullPath.c_str(), &fileStat) == 0)
        {
            if (S_ISDIR(fileStat.st_mode))
            {
                fileType = "Directory";
            }
            else
            {
                std::ostringstream sizeStr;
                sizeStr << fileStat.st_size;
                fileSize = sizeStr.str() + " bytes";
            }
        }

        html << "<tr>";
        html << "<td><a href=\"" << entry->d_name;
        if (fileType == "Directory") html << "/";
        html << "\">" << entry->d_name << "</a></td>";
        html << "<td>" << fileSize << "</td>";
        html << "<td>" << fileType << "</td>";
        html << "</tr>\n";
    }

    html << "</table>\n</body>\n</html>\n";
    closedir(dir);

    response.setStatus(200, "OK");
    response.setBody(html.str());
    response.setHeader("Content-Type", "text/html");
}

void RequestHandler::serveErrorPage(int errorCode, HttpResponse &response, const Config::ServerConfig &server)
{
    // Check for custom error pages
    for (size_t i = 0; i < server.errorPages.size(); ++i)
    {
        if (server.errorPages[i].errorCode == errorCode)
        {
            std::string errorPagePath = server.root + "/" + server.errorPages[i].filePath;
            if (fileExists(errorPagePath))
            {
                serveStaticFile(errorPagePath, response);
                response.setStatus(errorCode, HttpResponse::getDefaultStatusMessage(errorCode));
                return;
            }
        }
    }

    // Generate default error page
    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html>\n<head>\n";
    html << "<title>" << errorCode << " " << HttpResponse::getDefaultStatusMessage(errorCode) << "</title>\n";
    html << "</head>\n<body>\n";
    html << "<h1>" << errorCode << " " << HttpResponse::getDefaultStatusMessage(errorCode) << "</h1>\n";
    html << "<p>The requested resource could not be found or accessed.</p>\n";
    html << "<hr>\n";
    html << "<p><em>" << HttpResponse::serverName << "</em></p>\n";
    html << "</body>\n</html>\n";

    response.setStatus(errorCode, HttpResponse::getDefaultStatusMessage(errorCode));
    response.setBody(html.str());
    response.setHeader("Content-Type", "text/html");
}

void RequestHandler::executeCgi(HttpResponse &response, const Config::LocationConfig &/* location */)
{
    // // Basic CGI implementation (simplified)
    // std::cout << "CGI execution requested for: " << location.cgiPass << std::endl;
    
    // // For now, return a simple response
    // response.setStatus(200, "OK");
    // response.setBody("CGI execution not fully implemented yet");
    // response.setHeader("Content-Type", "text/plain");
    CgiHandler cgi(response);
    // Implement CGI execution logic here
    cgi.ExecuteCgi("test.py", "/usr/bin/python3");

}

void RequestHandler::handleFileUpload(const HttpRequest &request, HttpResponse &response, 
                                    const Config::ServerConfig &server, const Config::LocationConfig &location)
{
    // Simplified file upload handling
    std::cout << "File upload requested" << std::endl;
    
    // For now, just save the body to a file
    std::string uploadDir = location.root.empty() ? server.root : location.root;
    uploadDir += "/uploads";
    
    // Create uploads directory if it doesn't exist
    mkdir(uploadDir.c_str(), 0755);
    
    std::string filename = uploadDir + "/upload_" + getCurrentTimestamp();
    if (createFile(filename, request.getBody()))
    {
        response.setStatus(201, "Created");
        response.setBody("File uploaded successfully");
        response.setHeader("Content-Type", "text/plain");
    }
    else
    {
        serveErrorPage(500, response, server);
    }
}

void RequestHandler::handleFormData(const HttpRequest &request, HttpResponse &response)
{
    std::cout << "Form data received: " << request.getBody() << std::endl;
    
    response.setStatus(200, "OK");
    response.setBody("Form data processed successfully");
    response.setHeader("Content-Type", "text/plain");
}

std::string RequestHandler::resolveFilePath(const std::string &uri, const Config::LocationConfig &location, 
                                          const Config::ServerConfig &server) const
{
    std::string root = location.root.empty() ? server.root : location.root;
    if (root.empty())
        root = "./www"; // Default document root

    std::string path = root + uri;
    
    // Remove any double slashes
    size_t pos = 0;
    while ((pos = path.find("//", pos)) != std::string::npos)
    {
        path.replace(pos, 2, "/");
    }
    
    return path;
}

void RequestHandler::handleRedirect(HttpResponse &response, const Config::LocationConfig &location)
{
    response.setStatus(302, "Found");
    response.setHeader("Location", location.returnUrl);
    response.setBody("");
}

bool RequestHandler::fileExists(const std::string &path) const
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool RequestHandler::isDirectory(const std::string &path) const
{
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0)
        return false;
    return S_ISDIR(buffer.st_mode);
}

bool RequestHandler::hasPermission(const std::string &path) const
{
    return access(path.c_str(), R_OK) == 0;
}

bool RequestHandler::createFile(const std::string &path, const std::string &content) const
{
    std::ofstream file(path.c_str());
    if (!file.is_open())
        return false;
    
    file << content;
    file.close();
    return true;
}

std::string RequestHandler::getCurrentTimestamp() const
{
    std::time_t now = std::time(0);
    std::ostringstream oss;
    oss << now;
    return oss.str();
}

std::string RequestHandler::getMimeType(const std::string &filePath) const
{
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos)
    {
        return "application/octet-stream";
    }
    
    std::string extension = filePath.substr(dotPos + 1);
    
    // Convert to lowercase
    for (size_t i = 0; i < extension.length(); ++i)
    {
        if (extension[i] >= 'A' && extension[i] <= 'Z')
        {
            extension[i] = extension[i] + ('a' - 'A');
        }
    }
    
    if (extension == "html" || extension == "htm")
        return "text/html";
    else if (extension == "css")
        return "text/css";
    else if (extension == "js")
        return "application/javascript";
    else if (extension == "json")
        return "application/json";
    else if (extension == "png")
        return "image/png";
    else if (extension == "jpg" || extension == "jpeg")
        return "image/jpeg";
    else if (extension == "gif")
        return "image/gif";
    else if (extension == "txt")
        return "text/plain";
    else
        return "application/octet-stream";
}

// Helper methods for validation and configuration
bool RequestHandler::isMethodAllowed(const std::string &method, const Config::LocationConfig &location) const
{
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

    if (request.getMethod() == "POST")
    {
        std::string contentLength = request.getHeader("content-length");
        if (contentLength.empty())
        {
            std::cerr << "POST request missing Content-Length header" << std::endl;
            return false;
        }
    }

    return true;
}

const Config::ServerConfig &RequestHandler::findServerConfig(const HttpRequest &request) const
{
    std::string hostHeader = request.getHeader("host");
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

void RequestHandler::setErrorResponse(int statusCode, HttpResponse &response, const std::string &message)
{
    response.setStatus(statusCode, message);
    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html><head><title>" << statusCode << " " << message << "</title></head>\n";
    html << "<body><h1>" << statusCode << " " << message << "</h1></body></html>\n";
    response.setBody(html.str());
    response.setHeader("Content-Type", "text/html");
}