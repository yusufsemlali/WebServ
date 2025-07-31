#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <string>
#include <map>

class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();

    // Status management
    void setStatus(int code, const std::string &message = "");
    int getStatusCode() const;
    const std::string &getStatusMessage() const;

    // Header management
    void setHeader(const std::string &name, const std::string &value);
    std::string getHeader(const std::string &name) const;
    bool hasHeader(const std::string &name) const;
    void removeHeader(const std::string &name);

    // Body management
    void setBody(const std::string &content);
    void setBodyFromFile(const std::string &filePath);
    void appendBody(const std::string &content);
    void clearBody();
    const std::string &getBody() const;

    // Response generation
    std::string toString() const;
    bool isReady() const;
    void reset();

    // Static utility methods
    static std::string getDefaultStatusMessage(int code);
    static void setServerName(const std::string &name);
    static std::string serverName;

private:
    int statusCode;
    std::string statusMessage;
    std::map<std::string, std::string> headers;
    std::string body;

    // Helper methods
    std::string getContentTypeFromPath(const std::string &filePath) const;
};

#endif // HTTP_RESPONSE_HPP