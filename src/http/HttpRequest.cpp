#include "HttpRequest.hpp"

#include <iostream>
#include <sstream>

HttpRequest::HttpRequest()
    : requestComplete(false),
      headersParsed(false)
{
}

HttpRequest::~HttpRequest()
{
}

bool HttpRequest::parseRequest(const std::string &rawRequest)
{
    if (!parseRequestLine(rawRequest))
    {
        requestComplete = false;
        return false;
    }

    // Validate method
    if (!isValidMethod())
    {
        std::cerr << "Invalid HTTP method: " << method << std::endl;
        requestComplete = false;
        return false;
    }

    if (!isValidVersion())
    {
        std::cerr << "Invalid HTTP version: " << version << std::endl;
        requestComplete = false;
        return false;
    }

    if (!isValidUri())
    {
        std::cerr << "Invalid URI: " << uri << std::endl;
        requestComplete = false;
        return false;
    }

    // If all validations pass
    requestComplete = true;
    return true;
}
bool HttpRequest::isValidUri() const
{
    // Must not be empty and must start with '/'
    if (uri.empty() || uri[0] != '/')
        return false;

    // Disallow directory traversal
    if (uri.find("..") != std::string::npos)
        return false;

    // Disallow spaces and control characters
    for (size_t i = 0; i < uri.size(); ++i)
    {
        if (uri[i] <= 0x1F || uri[i] == 0x7F || uri[i] == ' ')
            return false;
    }

    return true;
}

bool HttpRequest::isComplete() const
{
    return requestComplete;
}

void HttpRequest::reset()
{
    method.clear();
    uri.clear();
    version.clear();
    query.clear();
    headers.clear();
    body.clear();
    requestComplete = false;
    headersParsed = false;
}

const std::string &HttpRequest::getMethod() const
{
    return method;
}

const std::string &HttpRequest::getUri() const
{
    return uri;
}

const std::string &HttpRequest::getVersion() const
{
    return version;
}

const std::string &HttpRequest::getQuery() const
{
    return query;
}

const std::map<std::string, std::string> &HttpRequest::getHeaders() const
{
    return headers;
}

const std::string &HttpRequest::getBody() const
{
    return body;
}

std::string HttpRequest::getHeader(const std::string &name) const
{
    std::map<std::string, std::string>::const_iterator it = headers.find(name);
    if (it != headers.end())
        return it->second;
    return "";
}

bool HttpRequest::hasHeader(const std::string &name) const
{
    return headers.find(name) != headers.end();
}

size_t HttpRequest::getContentLength() const
{
    std::string contentLengthStr = getHeader("Content-Length");
    if (!contentLengthStr.empty())
    {
        std::istringstream iss(contentLengthStr);
        size_t length;
        iss >> length;
        return length;
    }
    return 0;
}

std::string HttpRequest::getContentType() const
{
    return getHeader("Content-Type");
}

bool HttpRequest::isValidMethod() const
{
    return method == "GET" || method == "POST" || method == "DELETE" ||
           method == "PUT" || method == "HEAD" || method == "OPTIONS";
}

bool HttpRequest::isValidVersion() const
{
    return version == "HTTP/1.0" || version == "HTTP/1.1";
}

bool HttpRequest::parseRequestLine(const std::string &line)
{
    std::istringstream iss(line);
    std::string method, uri, version;
    if (!(iss >> method >> uri >> version))
    {
        std::cerr << "Invalid request line: " << line << std::endl;
        return false;
    }

    this->method = method;
    this->uri = uri;
    this->version = version;

    return true;
}
