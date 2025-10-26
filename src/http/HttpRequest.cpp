#include "HttpRequest.hpp"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>

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

    if (!parseHeaders(rawRequest))
    {
        requestComplete = false;
        return false;
    }

    if (!parseBody(rawRequest))
    {
        requestComplete = false;
        return false;
    }

    requestComplete = true;
    return true;
}

bool HttpRequest::parseHeadersOnly(const std::string &rawRequest)
{
    if (!parseRequestLine(rawRequest))
    {
        return false;
    }

    if (!parseHeaders(rawRequest))
    {
        return false;
    }

    headersParsed = true;
    return true;
}

bool HttpRequest::isValidUri() const
{
    if (uri.empty() || uri[0] != '/')
        return false;

    if (uri.find("..") != std::string::npos)
        return false;

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
    queryParams.clear();
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

std::string HttpRequest::getQueryParam(const std::string &key) const
{
    std::map<std::string, std::string>::const_iterator it = queryParams.find(key);
    if (it != queryParams.end())
    {
        return it->second;
    }
    return "";
}

bool HttpRequest::hasQueryParam(const std::string &key) const
{
    return queryParams.find(key) != queryParams.end();
}

const std::map<std::string, std::string> &HttpRequest::getQueryParams() const
{
    return queryParams;
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
    std::map<std::string, std::string>::const_iterator it = headers.find(toLower(name));
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

bool HttpRequest::parseRequestLine(const std::string &rawRequest)
{
    size_t firstLineEnd = rawRequest.find("\r\n");
    if (firstLineEnd == std::string::npos)
    {
        firstLineEnd = rawRequest.find("\n");
        if (firstLineEnd == std::string::npos)
        {
            return false;
        }
    }

    std::string requestLine = rawRequest.substr(0, firstLineEnd);
    std::istringstream iss(requestLine);
    std::string method, fullUri, version;

    if (!(iss >> method >> fullUri >> version))
    {
        return false;
    }

    this->method = method;
    this->version = version;

    parseUri(fullUri);

    return true;
}

bool HttpRequest::parseHeaders(const std::string &rawRequest)
{
    size_t headerStart = rawRequest.find("\r\n");
    if (headerStart == std::string::npos)
    {
        headerStart = rawRequest.find("\n");
        if (headerStart == std::string::npos)
        {
            return false;
        }
        headerStart += 1;
    }
    else
    {
        headerStart += 2;
    }

    size_t headerEnd = rawRequest.find("\r\n\r\n", headerStart);
    if (headerEnd == std::string::npos)
    {
        headerEnd = rawRequest.find("\n\n", headerStart);
        if (headerEnd == std::string::npos)
        {
            headerEnd = rawRequest.length();
        }
    }

    std::string headerSection = rawRequest.substr(headerStart, headerEnd - headerStart);

    std::istringstream headerStream(headerSection);
    std::string line;

    while (std::getline(headerStream, line))
    {
        if (!line.empty() && line[line.length() - 1] == '\r')
        {
            line.erase(line.length() - 1);
        }

        if (!line.empty())
        {
            if (!parseHeader(line))
            {
                return false;
            }
        }
    }

    headersParsed = true;
    return true;
}

bool HttpRequest::parseBody(const std::string &rawRequest)
{
    size_t bodyStart = rawRequest.find("\r\n\r\n");
    if (bodyStart != std::string::npos)
    {
        bodyStart += 4;
    }
    else
    {
        bodyStart = rawRequest.find("\n\n");
        if (bodyStart != std::string::npos)
        {
            bodyStart += 2;
        }
        else
        {
            body.clear();
            return true;
        }
    }

    if (bodyStart < rawRequest.length())
    {
        size_t contentLength = getContentLength();
        if (contentLength > 0)
        {
            size_t availableBytes = rawRequest.length() - bodyStart;
            size_t bytesToRead = contentLength < availableBytes ? contentLength : availableBytes;
            body = rawRequest.substr(bodyStart, bytesToRead);
        }
        else
        {
            body = rawRequest.substr(bodyStart);
        }
    }
    else
    {
        body.clear();
    }

    return true;
}

bool HttpRequest::parseHeader(const std::string &line)
{
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos)
    {
        return false;  // Invalid header format
    }

    std::string name = line.substr(0, colonPos);
    std::string value = line.substr(colonPos + 1);

    // Trim whitespace from value
    size_t valueStart = value.find_first_not_of(" \t");
    if (valueStart != std::string::npos)
    {
        value = value.substr(valueStart);
    }
    else
    {
        value.clear();
    }

    size_t valueEnd = value.find_last_not_of(" \t");
    if (valueEnd != std::string::npos)
    {
        value = value.substr(0, valueEnd + 1);
    }

    // Convert header name to lowercase for case-insensitive lookup
    headers[toLower(name)] = value;

    return true;
}

void HttpRequest::parseUri(const std::string &fullUri)
{
    size_t queryPos = fullUri.find('?');
    if (queryPos != std::string::npos)
    {
        uri = fullUri.substr(0, queryPos);
        query = fullUri.substr(queryPos + 1);
        parseQueryString(query);
    }
    else
    {
        uri = fullUri;
        query.clear();
        queryParams.clear();
    }
}

void HttpRequest::parseQueryString(const std::string &queryString)
{
    queryParams.clear();
    
    if (queryString.empty())
    {
        return;
    }
    
    size_t start = 0;
    size_t end = 0;
    
    while (end != std::string::npos)
    {
        end = queryString.find('&', start);
        std::string pair = (end == std::string::npos) ? 
                          queryString.substr(start) : 
                          queryString.substr(start, end - start);
        
        if (!pair.empty())
        {
            size_t equalPos = pair.find('=');
            if (equalPos != std::string::npos)
            {
                std::string key = urlDecodeQuery(pair.substr(0, equalPos));
                std::string value = urlDecodeQuery(pair.substr(equalPos + 1));
                queryParams[key] = value;
            }
            else
            {
                std::string key = urlDecodeQuery(pair);
                queryParams[key] = "";
            }
        }
        
        start = (end == std::string::npos) ? end : end + 1;
    }
}

std::string HttpRequest::urlDecodeQuery(const std::string &str) const
{
    std::string result;
    result.reserve(str.length());
    
    for (size_t i = 0; i < str.length(); ++i)
    {
        if (str[i] == '%' && i + 2 < str.length())
        {
            char hex[3] = { str[i + 1], str[i + 2], '\0' };
            char *endptr;
            long value = strtol(hex, &endptr, 16);
            
            if (endptr == hex + 2)
            {
                result += static_cast<char>(value);
                i += 2;
            }
            else
            {
                result += str[i];
            }
        }
        else if (str[i] == '+')
        {
            result += ' ';
        }
        else
        {
            result += str[i];
        }
    }
    
    return result;
}

std::string HttpRequest::toLower(const std::string &str) const
{
    std::string result = str;
    for (size_t i = 0; i < result.length(); ++i)
    {
        if (result[i] >= 'A' && result[i] <= 'Z')
        {
            result[i] = result[i] + ('a' - 'A');
        }
    }
    return result;
}
