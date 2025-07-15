#include "HttpRequest.hpp"
#include <iostream>
#include <sstream>

HttpRequest::HttpRequest() : requestComplete(false), headersParsed(false)
{
    // TODO: Initialize HTTP request
}

HttpRequest::~HttpRequest()
{
    // TODO: Cleanup HTTP request
}

bool HttpRequest::parseRequest(const std::string &rawRequest)
{
    (void)rawRequest;
    // TODO: Parse HTTP request from raw string
    return false;
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

const std::string& HttpRequest::getMethod() const
{
    return method;
}

const std::string& HttpRequest::getUri() const
{
    return uri;
}

const std::string& HttpRequest::getVersion() const
{
    return version;
}

const std::string& HttpRequest::getQuery() const
{
    return query;
}

const std::map<std::string, std::string>& HttpRequest::getHeaders() const
{
    return headers;
}

const std::string& HttpRequest::getBody() const
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
    return version == "HTTP/1.1" || version == "HTTP/1.0";
}

bool HttpRequest::parseRequestLine(const std::string &line)
{
    (void)line;
    // TODO: Parse request line (method, URI, version)
    return false;
}
