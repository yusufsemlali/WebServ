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

    std::cout << "\n======================================" << std::endl;
    std::cout << "Processing " << method << " request for " << uri << std::endl;
    std::cout << "======================================" << std::endl;

    // 1. Validate request
    if (!isValidRequest(request))
    {
        setErrorResponse(400, response, "Bad Request");
        std::cout << "\n--- VALIDATION ERROR RESPONSE ---" << std::endl;
        // response.printResponse();
        return;
    }

    const Config::ServerConfig &serverConfig = findServerConfig(request);
    
    // Add safety check and debug info
    try 
    {
        const Config::LocationConfig &locationConfig = findLocationConfig(serverConfig, uri);
        std::cout << "Location found: " << locationConfig.path << std::endl;
        
        // 2. Check if method is allowed
        if (!isMethodAllowed(method, locationConfig))
        {
            serveErrorPage(405, response, serverConfig);
            std::cout << "\n--- METHOD NOT ALLOWED RESPONSE ---" << std::endl;
            // response.printResponse();
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
                std::cout << "\n--- PAYLOAD TOO LARGE RESPONSE ---" << std::endl;
                // response.printResponse();
                return;
            }
        }

        // 4. Handle redirects
        if (!locationConfig.returnUrl.empty())
        {
            handleRedirect(response, locationConfig);
            std::cout << "\n--- REDIRECT RESPONSE ---" << std::endl;
            // response.printResponse();
            return;
        }

        // 5. Process request based on method
        if (method == "GET")
        {
            std::cout << "Processing GET request..." << std::endl;
            processGetRequest(request, response, serverConfig, locationConfig);
        }
        else if (method == "POST")
        {
            std::cout << "Processing POST request..." << std::endl;
            processPostRequest(request, response, serverConfig, locationConfig);
        }
        else if (method == "DELETE")
        {
            std::cout << "Processing DELETE request..." << std::endl;
            processDeleteRequest(request, response, serverConfig, locationConfig);
        }
        else
        {
            serveErrorPage(501, response, serverConfig);
        }
        
        // PRINT THE RESPONSE AFTER PROCESSING
        std::cout << "\n--- FINAL RESPONSE GENERATED ---" << std::endl;
        // response.printResponse();
        
        // Also print raw HTTP response for debugging
        std::cout << "\n--- RAW HTTP RESPONSE ---" << std::endl;
        std::string rawResponse = response.toString();
        std::cout << "Total size: " << rawResponse.length() << " bytes" << std::endl;
        std::cout << "Raw response preview (first 300 chars):" << std::endl;
        std::string preview = rawResponse.substr(0, 300);
        for (size_t i = 0; i < preview.length(); ++i)
        {
            if (preview[i] == '\r') std::cout << "\\r";
            else if (preview[i] == '\n') std::cout << "\\n\n";
            else std::cout << preview[i];
        }
        if (rawResponse.length() > 300) std::cout << "\n... (truncated)";
        std::cout << "\n=========================\n" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in handleRequest: " << e.what() << std::endl;
        setErrorResponse(500, response, "Internal Server Error");
        
        // PRINT ERROR RESPONSE
        std::cout << "\n--- EXCEPTION ERROR RESPONSE ---" << std::endl;
        // response.printResponse();
        std::cout << "\n";
        return;
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
        std::cout << "GET: File not found: " << filePath << std::endl;
        serveErrorPage(404, response, server);
        return;
    }

    // Check if it's a directory
    if (isDirectory(filePath))
    {
        std::cout << "GET: Path is directory: " << filePath << std::endl;
        // Try to serve index file
        std::string indexFile = location.index.empty() ? server.index : location.index;
        if (indexFile.empty())
            indexFile = "index.html";
        
        std::string indexPath = filePath;
        if (indexPath[indexPath.length() - 1] != '/')
            indexPath += "/";
        indexPath += indexFile;

        std::cout << "GET: Looking for index file: " << indexPath << std::endl;
        if (fileExists(indexPath) && !isDirectory(indexPath))
        {
            std::cout << "GET: Serving index file: " << indexPath << std::endl;
            serveStaticFile(indexPath, response);
        }
        else if (location.autoindex || server.autoindex)
        {
            std::cout << "GET: Generating directory listing for: " << filePath << std::endl;
            serveDirectoryListing(filePath, response);
        }
        else
        {
            std::cout << "GET: Directory access forbidden (no index, no autoindex)" << std::endl;
            serveErrorPage(403, response, server);
        }
    }
    else
    {
        std::cout << "GET: Path is file: " << filePath << std::endl;
        
        // Check if CGI is configured for this location first
        if (!location.cgiPass.empty())
        {
            std::cout << "GET: CGI configured: " << location.cgiPass << std::endl;
            executeCgi(request, response, server, location);
            return;
        }
        
        // Serve static file
        if (hasPermission(filePath))
        {
            std::cout << "GET: File has read permission" << std::endl;
            serveStaticFile(filePath, response);
        }
        else
        {
            std::cout << "GET: File access forbidden (no read permission)" << std::endl;
            serveErrorPage(403, response, server);
        }
    }
}

void RequestHandler::processPostRequest(const HttpRequest &request, HttpResponse &response, 
                                      const Config::ServerConfig &server, const Config::LocationConfig &location)
{
    std::string uri = request.getUri();
    
    std::cout << "POST: Processing URI: " << uri << std::endl;
    std::cout << "POST: Content-Type: " << request.getContentType() << std::endl;
    std::cout << "POST: Content-Length: " << request.getContentLength() << std::endl;
    
    // Check if CGI is configured for this location
    if (!location.cgiPass.empty())
    {
        std::cout << "POST: CGI configured: " << location.cgiPass << std::endl;
        executeCgi(request, response, server, location);
        return;
    }

    // Handle file uploads or form data
    std::string contentType = request.getContentType();
    
    if (contentType.find("multipart/form-data") != std::string::npos)
    {
        std::cout << "POST: Handling file upload" << std::endl;
        handleFileUpload(request, response, server, location);
    }
    else if (contentType == "application/x-www-form-urlencoded")
    {
        std::cout << "POST: Handling form data" << std::endl;
        handleFormData(request, response);
    }
    else
    {
        std::cout << "POST: Creating/updating resource" << std::endl;
        // Simple POST - create/update resource
        std::string filePath = resolveFilePath(uri, location, server);
        
        std::cout << "POST: Target file path: " << filePath << std::endl;
        if (createFile(filePath, request.getBody()))
        {
            std::cout << "POST: File created successfully" << std::endl;
            response.setStatus(201, "Created");
            response.setBody("Resource created successfully");
            response.setHeader("Content-Type", "text/plain");
        }
        else
        {
            std::cout << "POST: Failed to create file" << std::endl;
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
        std::cout << "DELETE: File not found: " << filePath << std::endl;
        serveErrorPage(404, response, server);
        return;
    }

    // Check permissions
    if (!hasPermission(filePath))
    {
        std::cout << "DELETE: No permission to access: " << filePath << std::endl;
        serveErrorPage(403, response, server);
        return;
    }

    // Don't allow deleting directories for safety
    if (isDirectory(filePath))
    {
        std::cout << "DELETE: Cannot delete directory: " << filePath << std::endl;
        serveErrorPage(403, response, server);
        return;
    }

    // Attempt to delete the file
    std::cout << "DELETE: Attempting to remove file: " << filePath << std::endl;
    if (std::remove(filePath.c_str()) == 0)
    {
        std::cout << "DELETE: File deleted successfully: " << filePath << std::endl;
        response.setStatus(204, "No Content");
        response.setBody("");
    }
    else
    {
        std::cout << "DELETE: Failed to delete file: " << filePath << std::endl;
        serveErrorPage(500, response, server);
    }
}

void RequestHandler::serveStaticFile(const std::string &filePath, HttpResponse &response)
{
    std::cout << "STATIC: Opening file: " << filePath << std::endl;
    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        std::cout << "STATIC: Failed to open file: " << filePath << std::endl;
        response.setStatus(404, "Not Found");
        return;
    }

    // Read file content
    std::ostringstream oss;
    oss << file.rdbuf();
    file.close();

    std::string mimeType = getMimeType(filePath);
    
    // Set response
    response.setStatus(200, "OK");
    response.setBody(oss.str());
    response.setHeader("Content-Type", mimeType);
    
    std::cout << "STATIC: File served successfully:" << std::endl;
    std::cout << "  Path: " << filePath << std::endl;
    std::cout << "  Size: " << oss.str().length() << " bytes" << std::endl;
    std::cout << "  MIME Type: " << mimeType << std::endl;
}

void RequestHandler::serveDirectoryListing(const std::string &dirPath, HttpResponse &response)
{
    std::cout << "DIRECTORY: Generating listing for: " << dirPath << std::endl;
    DIR* dir = opendir(dirPath.c_str());
    if (!dir)
    {
        std::cout << "DIRECTORY: Failed to open directory: " << dirPath << std::endl;
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
    int fileCount = 0;
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
        fileCount++;
    }

    html << "</table>\n</body>\n</html>\n";
    closedir(dir);

    response.setStatus(200, "OK");
    response.setBody(html.str());
    response.setHeader("Content-Type", "text/html");
    
    std::cout << "DIRECTORY: Generated listing with " << fileCount << " entries" << std::endl;
    std::cout << "DIRECTORY: HTML size: " << html.str().length() << " bytes" << std::endl;
}

void RequestHandler::serveErrorPage(int errorCode, HttpResponse &response, const Config::ServerConfig &server)
{
    std::cout << "ERROR: Serving error page: " << errorCode << std::endl;
    
    // Check for custom error pages
    for (size_t i = 0; i < server.errorPages.size(); ++i)
    {
        if (server.errorPages[i].errorCode == errorCode)
        {
            std::string errorPagePath = server.root + "/" + server.errorPages[i].filePath;
            std::cout << "ERROR: Checking custom error page: " << errorPagePath << std::endl;
            if (fileExists(errorPagePath))
            {
                std::cout << "ERROR: Using custom error page: " << errorPagePath << std::endl;
                serveStaticFile(errorPagePath, response);
                response.setStatus(errorCode, HttpResponse::getDefaultStatusMessage(errorCode));
                return;
            }
        }
    }

    std::cout << "ERROR: Generating default error page for " << errorCode << std::endl;
    
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
    
    std::cout << "ERROR: Default error page generated (" << html.str().length() << " bytes)" << std::endl;
}

void RequestHandler::executeCgi(const HttpRequest &request, HttpResponse &response, 
                               const Config::ServerConfig &server, const Config::LocationConfig &location)
{
    // Basic CGI implementation
    std::cout << "CGI: Executing script with interpreter: " << location.cgiPass << std::endl;
    
    CgiHandler cgi(response);
    
    // Get the actual requested file path
    std::string uri = request.getUri();
    std::string scriptPath = resolveFilePath(uri, location, server);
    std::string interpreterPath = location.cgiPass;
    
    // Convert relative interpreter paths to absolute
    if (interpreterPath.substr(0, 2) == "./") {
        std::string relativePath = interpreterPath.substr(2);
        
        // Check if it's a system interpreter (starts with usr/bin, bin/, etc.)
        if (relativePath.substr(0, 7) == "usr/bin" || relativePath.substr(0, 4) == "bin/") {
            // First try as absolute path for system interpreters
            std::string absolutePath = "/" + relativePath;
            if (access(absolutePath.c_str(), X_OK) == 0) {
                interpreterPath = absolutePath;
                std::cout << "CGI: Using system interpreter: " << interpreterPath << std::endl;
            }
            // If system path fails, try as relative path for local scripts
            else if (access(relativePath.c_str(), X_OK) == 0) {
                interpreterPath = relativePath;
                std::cout << "CGI: Using local interpreter: " << interpreterPath << std::endl;
            }
            else {
                std::cout << "CGI: Interpreter not found at system or local path: " << relativePath << std::endl;
            }
        }
        else {
            // For other paths, keep as relative
            interpreterPath = relativePath;
        }
    }
    
    cgi.ExecuteCgi(scriptPath, interpreterPath, request);
}

void RequestHandler::handleFileUpload(const HttpRequest &request, HttpResponse &response, 
                                    const Config::ServerConfig &server, const Config::LocationConfig &location)
{
    // Simplified file upload handling
    std::cout << "UPLOAD: File upload requested" << std::endl;
    std::cout << "UPLOAD: Body size: " << request.getBody().length() << " bytes" << std::endl;
    
    // For now, just save the body to a file
    std::string uploadDir = location.root.empty() ? server.root : location.root;
    uploadDir += "/uploads";
    
    std::cout << "UPLOAD: Upload directory: " << uploadDir << std::endl;
    
    // Create uploads directory if it doesn't exist
    mkdir(uploadDir.c_str(), 0755);
    
    std::string filename = uploadDir + "/upload_" + getCurrentTimestamp();
    std::cout << "UPLOAD: Target filename: " << filename << std::endl;
    
    if (createFile(filename, request.getBody()))
    {
        std::cout << "UPLOAD: File uploaded successfully: " << filename << std::endl;
        response.setStatus(201, "Created");
        response.setBody("File uploaded successfully to " + filename);
        response.setHeader("Content-Type", "text/plain");
    }
    else
    {
        std::cout << "UPLOAD: Failed to save uploaded file" << std::endl;
        serveErrorPage(500, response, server);
    }
}

void RequestHandler::handleFormData(const HttpRequest &request, HttpResponse &response)
{
    std::cout << "FORM: Form data received" << std::endl;
    std::cout << "FORM: Data length: " << request.getBody().length() << " bytes" << std::endl;
    std::cout << "FORM: Data content: " << request.getBody() << std::endl;
    
    response.setStatus(200, "OK");
    response.setBody("Form data processed successfully\nReceived: " + request.getBody());
    response.setHeader("Content-Type", "text/plain");
    
    std::cout << "FORM: Response generated" << std::endl;
}

std::string RequestHandler::resolveFilePath(const std::string &uri, const Config::LocationConfig &location, 
                                          const Config::ServerConfig &server) const
{
    std::string root = location.root.empty() ? server.root : location.root;
    if (root.empty())
        root = "./www"; // Default document root

    std::string path = root + uri;
    
    std::cout << "RESOLVE: URI: " << uri << std::endl;
    std::cout << "RESOLVE: Root: " << root << std::endl;
    std::cout << "RESOLVE: Initial path: " << path << std::endl;
    
    // Remove any double slashes
    size_t pos = 0;
    while ((pos = path.find("//", pos)) != std::string::npos)
    {
        path.replace(pos, 2, "/");
    }
    
    std::cout << "RESOLVE: Final path: " << path << std::endl;
    return path;
}

void RequestHandler::handleRedirect(HttpResponse &response, const Config::LocationConfig &location)
{
    std::cout << "REDIRECT: Redirecting to: " << location.returnUrl << std::endl;
    response.setStatus(302, "Found");
    response.setHeader("Location", location.returnUrl);
    response.setBody("");
}

bool RequestHandler::fileExists(const std::string &path) const
{
    struct stat buffer;
    bool exists = (stat(path.c_str(), &buffer) == 0);
    std::cout << "FILE_CHECK: " << path << " exists: " << (exists ? "YES" : "NO") << std::endl;
    return exists;
}

bool RequestHandler::isDirectory(const std::string &path) const
{
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0)
        return false;
    bool isDir = S_ISDIR(buffer.st_mode);
    std::cout << "DIR_CHECK: " << path << " is directory: " << (isDir ? "YES" : "NO") << std::endl;
    return isDir;
}

bool RequestHandler::hasPermission(const std::string &path) const
{
    bool hasRead = access(path.c_str(), R_OK) == 0;
    std::cout << "PERM_CHECK: " << path << " readable: " << (hasRead ? "YES" : "NO") << std::endl;
    return hasRead;
}

bool RequestHandler::createFile(const std::string &path, const std::string &content) const
{
    std::cout << "CREATE_FILE: Creating file: " << path << std::endl;
    std::cout << "CREATE_FILE: Content size: " << content.length() << " bytes" << std::endl;
    
    std::ofstream file(path.c_str());
    if (!file.is_open())
    {
        std::cout << "CREATE_FILE: Failed to open file for writing" << std::endl;
        return false;
    }
    
    file << content;
    file.close();
    
    std::cout << "CREATE_FILE: File created successfully" << std::endl;
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
    
    std::cout << "MIME: " << filePath << " -> " << mimeType << std::endl;
    return mimeType;
}

// FIXED: Helper methods for validation and configuration
bool RequestHandler::isMethodAllowed(const std::string &method, const Config::LocationConfig &location) const
{
    // Add debug print to trace execution
    std::cout << "METHOD_CHECK: Checking method: '" << method << "' for location: '" << location.path << "'" << std::endl;
    std::cout << "METHOD_CHECK: Location has " << location.allowedMethods.size() << " configured methods" << std::endl;
    
    if (location.allowedMethods.empty())
    {
        std::cout << "METHOD_CHECK: No specific methods configured, using defaults" << std::endl;
        bool allowed = (method == "GET" || method == "POST" || method == "DELETE" || method == "HEAD");
        std::cout << "METHOD_CHECK: Default check result: " << (allowed ? "ALLOWED" : "NOT ALLOWED") << std::endl;
        return allowed;
    }
    
    std::cout << "METHOD_CHECK: Checking against configured methods:" << std::endl;
    for (std::set<std::string>::const_iterator it = location.allowedMethods.begin();
         it != location.allowedMethods.end(); ++it)
    {
        std::cout << "  - " << *it << std::endl;
    }
    
    bool allowed = location.allowedMethods.find(method) != location.allowedMethods.end();
    std::cout << "METHOD_CHECK: Method " << method << " is " << (allowed ? "ALLOWED" : "NOT ALLOWED") << std::endl;
    return allowed;
}

bool RequestHandler::isValidRequest(const HttpRequest &request) const
{
    std::cout << "VALIDATION: Checking request validity..." << std::endl;
    
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

    std::cout << "VALIDATION: Request is valid" << std::endl;
    return true;
}

const Config::ServerConfig &RequestHandler::findServerConfig(const HttpRequest &request) const
{
    std::string hostHeader = request.getHeader("host");
    std::string hostname = hostHeader;

    std::cout << "SERVER_FIND: Host header: '" << hostHeader << "'" << std::endl;

    size_t colonPos = hostname.find(':');
    if (colonPos != std::string::npos)
    {
        hostname = hostname.substr(0, colonPos);
    }

    std::cout << "SERVER_FIND: Hostname: '" << hostname << "'" << std::endl;

    const std::vector<Config::ServerConfig> &servers = config.servers;
    
    // Safety check
    if (servers.empty())
    {
        throw std::runtime_error("No server configurations available");
    }
    
    std::cout << "SERVER_FIND: Checking " << servers.size() << " server configurations" << std::endl;
    
    for (size_t i = 0; i < servers.size(); ++i)
    {
        const std::vector<std::string> &serverNames = servers[i].serverNames;
        std::cout << "SERVER_FIND: Server " << i << " has " << serverNames.size() << " names" << std::endl;
        for (size_t j = 0; j < serverNames.size(); ++j)
        {
            std::cout << "SERVER_FIND: Checking server name: '" << serverNames[j] << "'" << std::endl;
            if (serverNames[j] == hostname)
            {
                std::cout << "SERVER_FIND: Found matching server configuration" << std::endl;
                return servers[i];
            }
        }
    }

    std::cout << "SERVER_FIND: No matching server found, using default (first server)" << std::endl;
    return servers[0];
}

// FIXED: This is the main fix for the segfault
const Config::LocationConfig &RequestHandler::findLocationConfig(const Config::ServerConfig &server, const std::string &uri) const
{
    const std::vector<Config::LocationConfig> &locations = server.locations;
    
    std::cout << "LOCATION_FIND: Looking for location matching URI: " << uri << std::endl;
    std::cout << "LOCATION_FIND: Server has " << locations.size() << " locations" << std::endl;
    
    // CRITICAL FIX: Handle empty locations vector
    if (locations.empty())
    {
        std::cout << "LOCATION_FIND: Warning - Server has no location blocks, creating default location" << std::endl;
        
        // Create a static default location that persists beyond function scope
        static Config::LocationConfig defaultLocation;
        defaultLocation.path = "/";
        defaultLocation.root = "./www";
        defaultLocation.autoindex = false;
        defaultLocation.clientMaxBodySize = 0;
        // Don't set any specific methods - let it use defaults
        defaultLocation.allowedMethods.clear();
        
        std::cout << "LOCATION_FIND: Using default location: " << defaultLocation.path << std::endl;
        return defaultLocation;
    }
    
    const Config::LocationConfig *bestMatch = NULL;
    size_t bestMatchLength = 0;

    for (size_t i = 0; i < locations.size(); ++i)
    {
        const std::string &locationPath = locations[i].path;
        std::cout << "LOCATION_FIND: Checking location[" << i << "]: " << locationPath << std::endl;
        
        bool matches = false;
        
        // Check for wildcard patterns (e.g., *.py, *.php)
        if (locationPath.length() > 0 && locationPath[0] == '*')
        {
            // Extract the extension from the pattern (e.g., ".py" from "*.py")
            std::string extension = locationPath.substr(1);
            
            // Check if URI ends with this extension
            if (uri.length() >= extension.length() && 
                uri.substr(uri.length() - extension.length()) == extension)
            {
                matches = true;
                std::cout << "LOCATION_FIND: Wildcard match found for extension: " << extension << std::endl;
            }
        }
        // Check for exact prefix match (e.g., "/download", "/bin")
        else if (uri.find(locationPath) == 0)
        {
            matches = true;
            std::cout << "LOCATION_FIND: Prefix match found" << std::endl;
        }
        
        if (matches && locationPath.length() > bestMatchLength)
        {
            bestMatch = &locations[i];
            bestMatchLength = locationPath.length();
            std::cout << "LOCATION_FIND: New best match (length: " << bestMatchLength << ")" << std::endl;
        }
    }

    if (bestMatch)
    {
        std::cout << "LOCATION_FIND: Found best location match: " << bestMatch->path << std::endl;
        return *bestMatch;
    }

    std::cout << "LOCATION_FIND: No specific match found, using first location: " << locations[0].path << std::endl;
    return locations[0];
}

void RequestHandler::setErrorResponse(int statusCode, HttpResponse &response, const std::string &message)
{
    std::cout << "ERROR_RESPONSE: Setting error response: " << statusCode << " " << message << std::endl;
    
    response.setStatus(statusCode, message);
    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html><head><title>" << statusCode << " " << message << "</title></head>\n";
    html << "<body><h1>" << statusCode << " " << message << "</h1></body></html>\n";
    response.setBody(html.str());
    response.setHeader("Content-Type", "text/html");
    
    std::cout << "ERROR_RESPONSE: Error response generated (" << html.str().length() << " bytes)" << std::endl;
}