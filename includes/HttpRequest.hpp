#pragma once

#include <string>
#include <map>
#include <vector>

class HttpRequest
{
public:
    HttpRequest();
    ~HttpRequest();

    // Request parsing
    bool parseRequest(const std::string &rawRequest);
    bool isComplete() const;
    void reset();

    // Getters
    const std::string& getMethod() const;
    const std::string& getUri() const;
    const std::string& getVersion() const;
    const std::string& getQuery() const;
    const std::map<std::string, std::string>& getHeaders() const;
    const std::string& getBody() const;
    
    // Header operations
    std::string getHeader(const std::string &name) const;
    bool hasHeader(const std::string &name) const;
    
    // Content operations
    size_t getContentLength() const;
    std::string getContentType() const;
    
    // Validation
    bool isValidMethod() const;
    bool isValidVersion() const;

private:
    std::string method;
    std::string uri;
    std::string version;
    std::string query;
    std::map<std::string, std::string> headers;
    std::string body;
    
    bool requestComplete;
    bool headersParsed;
    
    // Parsing helpers
    bool parseRequestLine(const std::string &line);
    bool parseHeader(const std::string &line);
    void parseUri(const std::string &fullUri);
    std::string toLower(const std::string &str) const;
};
