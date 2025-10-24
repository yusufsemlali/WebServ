#include "HttpResponse.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>

std::string HttpResponse::serverName = "WebServ/1.0";

HttpResponse::HttpResponse() : statusCode(200), statusMessage("OK")
{
    setHeader("Server", serverName);
    setHeader("Connection", "close");
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::setStatus(int code, const std::string &message)
{
    statusCode = code;
    if (message.empty())
    {
        statusMessage = getDefaultStatusMessage(code);
    }
    else
    {
        statusMessage = message;
    }
}

void HttpResponse::setHeader(const std::string &name, const std::string &value)
{
    headers[name] = value;
}

void HttpResponse::setBody(const std::string &content)
{
    body = content;
    std::ostringstream oss;
    oss << content.length();
    setHeader("Content-Length", oss.str());
}

void HttpResponse::setBodyFromFile(const std::string &filePath)
{
    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (file.is_open())
    {
        std::ostringstream oss;
        oss << file.rdbuf();
        setBody(oss.str());
        file.close();
        
        setHeader("Content-Type", getContentTypeFromPath(filePath));
    }
}

void HttpResponse::appendBody(const std::string &content)
{
    body += content;
    std::ostringstream oss;
    oss << body.length();
    setHeader("Content-Length", oss.str());
}

void HttpResponse::clearBody()
{
    body.clear();
    setHeader("Content-Length", "0");
}

std::string HttpResponse::toString() const
{
    std::ostringstream response;
    
    response << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
    
    std::time_t now = std::time(0);
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&now));
    response << "Date: " << timeStr << "\r\n";
    
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it)
    {
        response << it->first << ": " << it->second << "\r\n";
    }
    
    response << "\r\n";
    
    response << body;
    
    std::string result = response.str();

#ifdef VERBOSE_LOGGING
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it)
    {
        std::cout << "  " << it->first << ": " << it->second << std::endl;
    }
    if (!body.empty())
    {
        if (body.length() > 100) std::cout << "...";
        std::cout << std::endl;
    }
#endif

    return result;
}

const std::string& HttpResponse::getBody() const
{
    return body;
}

int HttpResponse::getStatusCode() const
{
    return statusCode;
}

const std::string& HttpResponse::getStatusMessage() const
{
    return statusMessage;
}

std::string HttpResponse::getHeader(const std::string &name) const
{
    std::map<std::string, std::string>::const_iterator it = headers.find(name);
    if (it != headers.end())
    {
        return it->second;
    }
    return "";
}

bool HttpResponse::hasHeader(const std::string &name) const
{
    return headers.find(name) != headers.end();
}

void HttpResponse::removeHeader(const std::string &name)
{
    headers.erase(name);
}

void HttpResponse::reset()
{
    statusCode = 200;
    statusMessage = "OK";
    headers.clear();
    body.clear();
    
    setHeader("Server", serverName);
    setHeader("Connection", "close");
}

bool HttpResponse::isReady() const
{
    return !body.empty() || statusCode >= 400;
}

std::string HttpResponse::getDefaultStatusMessage(int code)
{
    switch (code)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default: return "Unknown";
    }
}

void HttpResponse::setServerName(const std::string &name)
{
    serverName = name;
}

std::string HttpResponse::getContentTypeFromPath(const std::string &filePath) const
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
    else if (extension == "ico")
        return "image/x-icon";
    else if (extension == "txt")
        return "text/plain";
    else if (extension == "pdf")
        return "application/pdf";
    else if (extension == "xml")
        return "application/xml";
    else
        return "application/octet-stream";
}