#include "RequestHandler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <cstdio>
#include "CgiHandler.hpp"
#include "CgiOperation.hpp"
#include "ClientConnection.hpp"

RequestHandler::RequestHandler(const Config &config, SocketManager &socketManager)
    : config(config), socketManager(socketManager)
{
}

RequestHandler::~RequestHandler()
{
}

void RequestHandler::handleRequest(const HttpRequest &request, HttpResponse &response, ClientConnection* connection)
{
    std::string method = request.getMethod();
    std::string uri = request.getUri();

    std::cout << "Processing " << method << " request for " << uri << " from client " << connection->getClientAddress() << std::endl;

    if (!isValidRequest(request))
    {
        generateErrorPage(400, response, connection->getServerFd());
        return;
    }

    int serverFd = -1;
    if (connection) {
        serverFd = connection->getServerFd();
    }
    
    const Config::ServerConfig &serverConfig = findServerConfig(request, serverFd);
    
    try 
    {
        const Config::LocationConfig &locationConfig = findLocationConfig(serverConfig, uri);
        
        if (!isMethodAllowed(method, locationConfig))
        {
            serveErrorPage(405, response, serverConfig);
            return;
        }

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

        if (!locationConfig.returnUrl.empty())
        {
            handleRedirect(response, locationConfig);
            return;
        }

        if (method == "GET")
        {
            processGetRequest(request, response, serverConfig, locationConfig, connection);
        }
        else if (method == "POST")
        {
            processPostRequest(request, response, serverConfig, locationConfig, connection);
        }
        else if (method == "DELETE")
        {
            processDeleteRequest(request, response, serverConfig, locationConfig, connection);
        }
        else
        {
            serveErrorPage(501, response, serverConfig);
        }
        
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in handleRequest: " << e.what() << std::endl;
        generateErrorPage(500, response, connection->getServerFd());
        return;
    }
}

void RequestHandler::generateErrorPage(int errorCode, HttpResponse &response, int serverFd)
{
    const std::vector<const Config::ServerConfig*>* serverConfigs = socketManager.getServerConfigs(serverFd);
    
    if (serverConfigs && !serverConfigs->empty())
    {
        const Config::ServerConfig& server = *(serverConfigs->at(0));
        serveErrorPage(errorCode, response, server);
    }
    else if (!config.servers.empty())
    {
        serveErrorPage(errorCode, response, config.servers[0]);
    }
    else
    {
        setErrorResponse(errorCode, response, HttpResponse::getDefaultStatusMessage(errorCode));
    }
}

void RequestHandler::processGetRequest(const HttpRequest &request, HttpResponse &response, 
                                     const Config::ServerConfig &server, const Config::LocationConfig &location,
                                     ClientConnection* connection)
{
    std::string uri = request.getUri();
    std::string filePath = resolveFilePath(uri, location, server);
    

    if (!fileExists(filePath))
    {
        serveErrorPage(404, response, server);
        return;
    }

    if (isDirectory(filePath))
    {
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
        
        if (!location.cgiPass.empty())
        {
            executeCgi(request, response, server, location, connection);
            return;
        }     
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
                                      const Config::ServerConfig &server, const Config::LocationConfig &location,
                                      ClientConnection* connection)
{
    std::string uri = request.getUri();
    
    if (!location.cgiPass.empty())
    {
        executeCgi(request, response, server, location, connection);
        return;
    }

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
                                        const Config::ServerConfig &server, const Config::LocationConfig &location,
                                        ClientConnection* /* connection */)
{
    std::string uri = request.getUri();
    std::string filePath = resolveFilePath(uri, location, server);
    

    if (!fileExists(filePath))
    {
        serveErrorPage(404, response, server);
        return;
    }

    if (!hasPermission(filePath))
    {
        serveErrorPage(403, response, server);
        return;
    }
    if (isDirectory(filePath))
    {
        serveErrorPage(403, response, server);
        return;
    }

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

    std::ostringstream oss;
    oss << file.rdbuf();
    file.close();

    std::string mimeType = getMimeType(filePath);
    
    response.setStatus(200, "OK");
    response.setBody(oss.str());
    response.setHeader("Content-Type", mimeType);
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
    for (size_t i = 0; i < server.errorPages.size(); ++i)
    {
        if (server.errorPages[i].errorCode == errorCode)
        {
            std::string errorPagePath = server.errorPages[i].filePath;
            if (fileExists(errorPagePath))
            {
                serveStaticFile(errorPagePath, response);
                response.setStatus(errorCode, HttpResponse::getDefaultStatusMessage(errorCode));
                return;
            }
        }
    }

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

void RequestHandler::executeCgi(const HttpRequest &request, HttpResponse &response, 
                               const Config::ServerConfig &server, const Config::LocationConfig &location,
                               ClientConnection* connection)
{
    std::string uri = request.getUri();
    std::string scriptPath = resolveFilePath(uri, location, server);
    std::string interpreterPath = location.cgiPass;
    
    if (interpreterPath.substr(0, 2) == "./") {
        std::string relativePath = interpreterPath.substr(2);
        
        if (relativePath.substr(0, 7) == "usr/bin" || relativePath.substr(0, 4) == "bin/") {
            std::string absolutePath = "/" + relativePath;
            if (access(absolutePath.c_str(), X_OK) == 0) {
                interpreterPath = absolutePath;
            }
            else if (access(relativePath.c_str(), X_OK) == 0) {
                interpreterPath = relativePath;
            }
            else {
                response.setStatus(500, "Internal Server Error");
                response.setBody("CGI interpreter not found");
                response.setHeader("Content-Type", "text/plain");
                return;
            }
        }
        else {
            interpreterPath = relativePath;
        }
    }
    
    if (connection != NULL) {
        
        std::string documentRoot = location.root.empty() ? server.root : location.root;
        
        // Extract port from Host header or use first configured port
        std::string serverPort = "8080";  // default
        std::string hostHeader = request.getHeader("Host");
        size_t colonPos = hostHeader.find(':');
        if (colonPos != std::string::npos) {
            serverPort = hostHeader.substr(colonPos + 1);
        } else if (!server.listenConfigs.empty()) {
            serverPort = server.listenConfigs[0].port;
        }
        
        std::string clientAddr = connection->getClientAddress();
        
        CgiOperation* cgiOp = new CgiOperation(scriptPath, interpreterPath, request, documentRoot, 
                                               serverPort, clientAddr);
        
        if (cgiOp->hasError()) {
            std::cerr << "CGI: Failed to start CGI operation: " << cgiOp->getError() << std::endl;
            response.setStatus(500, "Internal Server Error");
            response.setBody("Failed to start CGI script: " + cgiOp->getError());
            response.setHeader("Content-Type", "text/plain");
            delete cgiOp;
            return;
        }
        
        connection->setPendingOperation(cgiOp);
    } else {
        CgiHandler cgi(response);
        cgi.ExecuteCgi(scriptPath, interpreterPath, request);
    }
}

void RequestHandler::handleFileUpload(const HttpRequest &request, HttpResponse &response, 
                                    const Config::ServerConfig &server, const Config::LocationConfig &location)
{
    std::string uploadDir = location.root.empty() ? server.root : location.root;
    uploadDir += "/uploads";
    
    mkdir(uploadDir.c_str(), 0755);
    
    std::string filename = uploadDir + "/upload_" + getCurrentTimestamp();
    
    if (createFile(filename, request.getBody()))
    {
        response.setStatus(201, "Created");
        response.setBody("File uploaded successfully to " + filename);
        response.setHeader("Content-Type", "text/plain");
    }
    else
    {
        serveErrorPage(500, response, server);
    }
}

void RequestHandler::handleFormData(const HttpRequest &request, HttpResponse &response)
{
    response.setStatus(200, "OK");
    response.setBody("Form data processed successfully\nReceived: " + request.getBody());
    response.setHeader("Content-Type", "text/plain");
}

std::string RequestHandler::resolveFilePath(const std::string &uri, const Config::LocationConfig &location, 
                                          const Config::ServerConfig &server) const
{
    std::string root = location.root.empty() ? server.root : location.root;
    if (root.empty())
        root = "./www"; // Default document root

    std::string path = root + uri;
    
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
    bool exists = (stat(path.c_str(), &buffer) == 0);
    return exists;
}

bool RequestHandler::isDirectory(const std::string &path) const
{
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0)
        return false;
    bool isDir = S_ISDIR(buffer.st_mode);
    return isDir;
}

bool RequestHandler::hasPermission(const std::string &path) const
{
    bool hasRead = access(path.c_str(), R_OK) == 0;
    return hasRead;
}

bool RequestHandler::createFile(const std::string &path, const std::string &content) const
{
    std::ofstream file(path.c_str());
    if (!file.is_open())
    {
        return false;
    }
    
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
    
    for (size_t i = 0; i < extension.length(); ++i)
    {
        if (extension[i] >= 'A' && extension[i] <= 'Z')
        {
            extension[i] = extension[i] + ('a' - 'A');
        }
    }
    
    std::string mimeType;
    if (extension == "html" || extension == "htm")
        mimeType = "text/html";
    else if (extension == "css")
        mimeType = "text/css";
    else if (extension == "js")
        mimeType = "application/javascript";
    else if (extension == "json")
        mimeType = "application/json";
    else if (extension == "png")
        mimeType = "image/png";
    else if (extension == "jpg" || extension == "jpeg")
        mimeType = "image/jpeg";
    else if (extension == "gif")
        mimeType = "image/gif";
    else if (extension == "txt")
        mimeType = "text/plain";
    else
        mimeType = "application/octet-stream";
    
    return mimeType;
}

bool RequestHandler::isMethodAllowed(const std::string &method, const Config::LocationConfig &location) const
{
    
    if (location.allowedMethods.empty())
    {
        bool allowed = (method == "GET" || method == "POST" || method == "DELETE" || method == "HEAD");
        return allowed;
    }
    
    bool allowed = location.allowedMethods.find(method) != location.allowedMethods.end();
    return allowed;
}

bool RequestHandler::isValidRequest(const HttpRequest &request) const
{

    if (!request.isComplete())
    {
        std::cerr << "VALIDATION: Request parsing incomplete" << std::endl;
        return false;
    }

    if (!request.isValidMethod())
    {
        std::cerr << "VALIDATION: Invalid HTTP method: " << request.getMethod() << std::endl;
        return false;
    }

    if (!request.isValidVersion())
    {
        std::cerr << "VALIDATION: Invalid HTTP version: " << request.getVersion() << std::endl;
        return false;
    }

    if (!request.isValidUri())
    {
        std::cerr << "VALIDATION: Invalid URI: " << request.getUri() << std::endl;
        return false;
    }

    if (request.getMethod() == "POST")
    {
        std::string contentLength = request.getHeader("content-length");
        if (contentLength.empty())
        {
            std::cerr << "VALIDATION: POST request missing Content-Length header" << std::endl;
            return false;
        }
    }

    return true;
}

const Config::ServerConfig &RequestHandler::findServerConfig(const HttpRequest &request, int serverFd) const
{
    std::string hostHeader = request.getHeader("host");
    
    if (hostHeader.empty())
    {
        const std::vector<const Config::ServerConfig*>* serverConfigs = socketManager.getServerConfigs(serverFd);
        if (serverConfigs && !serverConfigs->empty())
        {
            return *(serverConfigs->at(0));
        }
        if (!config.servers.empty())
        {
            return config.servers[0];
        }
        throw std::runtime_error("No server configurations available");
    }

    std::string hostname = hostHeader;
    size_t colonPos = hostname.find(':');
    if (colonPos != std::string::npos)
    {
        hostname = hostname.substr(0, colonPos);
    }
    
    hostname = toLower(hostname);

    const std::vector<const Config::ServerConfig*>* serverConfigs = socketManager.getServerConfigs(serverFd);
    
    if (!serverConfigs || serverConfigs->empty())
    {
        if (!config.servers.empty())
        {
            return config.servers[0];
        }
        throw std::runtime_error("No server configurations available");
    }
    
    for (size_t i = 0; i < serverConfigs->size(); ++i)
    {
        const Config::ServerConfig* server = serverConfigs->at(i);
        const std::vector<std::string> &serverNames = server->serverNames;
        
        for (size_t j = 0; j < serverNames.size(); ++j)
        {
            std::string serverName = toLower(serverNames[j]);
            
            if (serverName == hostname)
            {
                return *server;
            }
        }
    }
    return *(serverConfigs->at(0));
}

std::string RequestHandler::toLower(const std::string &str) const
{
    std::string result = str;
    for (size_t i = 0; i < result.length(); ++i)
    {
        result[i] = std::tolower(static_cast<unsigned char>(result[i]));
    }
    return result;
}

const Config::LocationConfig &RequestHandler::findLocationConfig(const Config::ServerConfig &server, const std::string &uri) const
{
    const std::vector<Config::LocationConfig> &locations = server.locations;
    
    if (locations.empty())
    {
        
        static Config::LocationConfig defaultLocation;
        defaultLocation.path = "/";
        defaultLocation.root = "./www";
        defaultLocation.autoindex = false;
        defaultLocation.clientMaxBodySize = 0;
        defaultLocation.allowedMethods.clear();
        
        return defaultLocation;
    }
    
    const Config::LocationConfig *bestMatch = NULL;
    size_t bestMatchLength = 0;

    for (size_t i = 0; i < locations.size(); ++i)
    {
        const std::string &locationPath = locations[i].path;
        
        bool matches = false;
        
        if (locationPath.length() > 0 && locationPath[0] == '*')
        {
            std::string extension = locationPath.substr(1); 
            if (uri.length() >= extension.length() && 
                uri.substr(uri.length() - extension.length()) == extension)
            {
                matches = true;
            }
        }
        
        else if (uri.find(locationPath) == 0)
        {
            matches = true;
        }
        
        if (matches && locationPath.length() > bestMatchLength)
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