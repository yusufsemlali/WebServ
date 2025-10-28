#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>

class HttpResponse
{
private:
    int statusCode;
    std::string statusMessage;
    std::map<std::string, std::string> headers;
    std::string body;

public:
    static std::string serverName;

    HttpResponse();
    ~HttpResponse();

    void setStatus(int code, const std::string &message = "");
    int getStatusCode() const;
    const std::string& getStatusMessage() const;
    void setHeader(const std::string &name, const std::string &value);
    std::string getHeader(const std::string &name) const;
    bool hasHeader(const std::string &name) const;
    void removeHeader(const std::string &name);

    void setBody(const std::string &content);
    void setBodyFromFile(const std::string &filePath);
    void appendBody(const std::string &content);
    void clearBody();
    const std::string& getBody() const;

    std::string toString() const;
    void reset();
    bool isReady() const;

    static std::string getDefaultStatusMessage(int code);
    static void setServerName(const std::string &name);

    void printResponse() const;
    void printHeaders() const;
    void printStatus() const;
    std::string getResponseSummary() const;

private:
    std::string getContentTypeFromPath(const std::string &filePath) const;
};

void logResponse(const HttpResponse& response, const std::string& context = "");

#endif // HTTPRESPONSE_HPP