#pragma once

#include <ctime>
#include <string>

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "RequestHandler.hpp"
#include "Config.hpp"

class ClientConnection
{
   public:
    ClientConnection(int socketFd, const struct sockaddr_in &clientAddr, RequestHandler &handler, const Config::ServerConfig* serverConfig);
    ~ClientConnection();

    // Connection management
    bool readData();
    bool writeData();
    void close();
    bool isConnected() const;

    // Request/Response handling
    bool hasCompleteRequest() const;
    HttpRequest &getCurrentRequest();
    HttpResponse &getCurrentResponse();

    // Connection state
    bool isKeepAlive() const;
    void setKeepAlive(bool keepAlive);
    bool isReadyToWrite() const;
    bool isReadyToRead() const;

    // Timeout management
    void updateLastActivity();
    bool isTimedOut(int timeoutSeconds) const;

    // Socket operations
    int getSocketFd() const;
    std::string getClientAddress() const;

    // Buffer management
    void clearBuffers();
    size_t getBytesRead() const;
    size_t getBytesWritten() const;

   private:
    int socketFd;
    std::string clientAddress;
    const Config::ServerConfig* serverConfig;  // Server config from accepting socket

    std::string readBuffer;
    std::string writeBuffer;

    HttpRequest currentRequest;
    HttpResponse currentResponse;

    RequestHandler &handleRequest;

    bool connected;
    bool keepAlive;
    time_t lastActivity;

    size_t bytesRead;
    size_t bytesWritten;
    size_t writeOffset;

    static const size_t MAX_BUFFER_SIZE = 8192;

    // Helper methods
    bool processReadBuffer();
    void serveStaticFile(const std::string &requestPath);
    std::string getContentType(const std::string &filePath);
    void serve404();
};
