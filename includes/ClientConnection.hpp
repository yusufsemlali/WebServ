#pragma once

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <string>
#include <ctime>

class ClientConnection
{
public:
    ClientConnection(int socketFd);
    ~ClientConnection();

    // Connection management
    bool readData();
    bool writeData();
    void close();
    bool isConnected() const;
    
    // Request/Response handling
    bool hasCompleteRequest() const;
    HttpRequest& getCurrentRequest();
    HttpResponse& getCurrentResponse();
    
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
    
    std::string readBuffer;
    std::string writeBuffer;
    
    HttpRequest currentRequest;
    HttpResponse currentResponse;
    
    bool connected;
    bool keepAlive;
    time_t lastActivity;
    
    size_t bytesRead;
    size_t bytesWritten;
    size_t writeOffset;
    
    static const size_t MAX_BUFFER_SIZE = 8192;
    
    // Helper methods
    void parseClientAddress();
    bool processReadBuffer();
};
