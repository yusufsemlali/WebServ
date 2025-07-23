#include "HttpResponse.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

std::string HttpResponse::serverName = "WebServ/1.0";

HttpResponse::HttpResponse() : statusCode(200), statusMessage("OK")
{
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::setStatus(int code, const std::string &message)
{
    statusCode = code;
    if (message.empty())
        statusMessage = getDefaultStatusMessage(code);
    else
        statusMessage = message;
}

void HttpResponse::setHeader(const std::string &name, const std::string &value)
{
    headers[name] = value;
}

void HttpResponse::setBody(const std::string &content)
{
    body = content;
    // Update Content-Length header
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
    }
}

void HttpResponse::appendBody(const std::string &content)
{
    body += content;
    // Update Content-Length header
    std::ostringstream oss;
    oss << body.length();
    setHeader("Content-Length", oss.str());
}

void HttpResponse::clearBody()
{
    body.clear();
    setHeader("Content-Length", "0");
}

std::string
HttpResponse::toString() const
{
    std::ostringstream response;

    // Status line
    response << "HTTP/1.0 " << statusCode << " " << statusMessage << "\r\n";

    // Headers
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it)
    {
        response << it->first << ": " << it->second << "\r\n";
    }

    // Server header
    response << "Server: " << serverName << "\r\n";

    // Empty line
    response << "\r\n";

    // Body
    response << body;

    return response.str();
}

const std::string &
HttpResponse::getBody() const
{
    return body;
}

int HttpResponse::getStatusCode() const
{
    return statusCode;
}

const std::string &
HttpResponse::getStatusMessage() const
{
    return statusMessage;
}

std::string
HttpResponse::getHeader(const std::string &name) const
{
    std::map<std::string, std::string>::const_iterator it = headers.find(name);
    if (it != headers.end())
        return it->second;
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
}

bool HttpResponse::isReady() const
{
    return statusCode != 0;
}

std::string
HttpResponse::getDefaultStatusMessage(int code)
{
    switch (code)
    {
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 204:
            return "No Content";
        case 301:
            return "Moved Permanently";
        case 302:
            return "Found";
        case 304:
            return "Not Modified";
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 413:
            return "Payload Too Large";
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        case 502:
            return "Bad Gateway";
        case 503:
            return "Service Unavailable";
        default:
            return "Unknown";
    }
}

void HttpResponse::setServerName(const std::string &name)
{
    serverName = name;
}
