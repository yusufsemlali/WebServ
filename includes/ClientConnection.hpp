#pragma once

#include <sys/socket.h>
#include <string>

#include "AsyncOperation.hpp"
#include "RequestBodyBuffer.hpp"
#include "HttpResponse.hpp"
#include "RequestHandler.hpp"
#include "AsyncOperation.hpp"

enum ConnectionState {
    READING_REQUEST,     
    PROCESSING_REQUEST,  
    WAITING_ASYNC,       
    WRITING_RESPONSE,    
    KEEP_ALIVE,          
    CLOSING              
};

struct RequestContext {
    HttpRequest request;
    HttpResponse response;
    ConnectionState state;
    
    AsyncOperation* pendingOperation;
    
    time_t startTime;
    time_t lastActivity;
    
    RequestContext() : state(READING_REQUEST), pendingOperation(NULL), 
                      startTime(time(NULL)), lastActivity(time(NULL)) {}
};

class ClientConnection
{
   public:
    ClientConnection(int socketFd, const struct sockaddr_in &clientAddr, RequestHandler &handler);
    ~ClientConnection();

    bool readData();
    bool writeData();
    void close();
    bool isConnected() const;
    void setServerFd(int serverFd);
    int getServerFd() const;

    bool hasCompleteHeaders() const;
    bool hasCompleteRequest() const;
    HttpRequest &getCurrentRequest();
    HttpResponse &getCurrentResponse();

    bool isKeepAlive() const;
    void setKeepAlive(bool keepAlive);
    bool isReadyToWrite() const;
    bool isReadyToRead() const;

    void updateLastActivity();
    bool isTimedOut(int timeoutSeconds) const;

    int getSocketFd() const;
    std::string getClientAddress() const;

    void clearBuffers();
    size_t getBytesRead() const;
    size_t getBytesWritten() const;
    
    ConnectionState getState() const;
    void setState(ConnectionState newState);
    
    void setPendingOperation(AsyncOperation* operation);
    void completePendingOperation();
    bool hasPendingOperation() const;
    AsyncOperation* getPendingOperation() const;
    
    bool canRead() const;
    bool canWrite() const;
    bool isReadyForCleanup() const;
    
    std::string getRequestBodyTempFile() const;

   private:
    int socketFd;
    int serverFd;
    std::string clientAddress;

    std::string readBuffer;
    std::string writeBuffer;

    RequestContext context;
    RequestHandler &handleRequest;

    bool connected;
    bool keepAlive;
    bool headersValidated;
    time_t lastActivity;

    size_t bytesRead;
    size_t bytesWritten;
    size_t writeOffset;

    static const size_t MAX_BUFFER_SIZE = 8192;
    
    RequestBodyBuffer bodyBuffer;

    bool processReadBuffer();
    size_t findHeaderEnd(const std::string& buffer, size_t& headerEndLen) const;
    void serveStaticFile(const std::string &requestPath);
    std::string getContentType(const std::string &filePath);
    void serve404();
    void parseCgiOutput(const std::string& output);
    void rejectRequestEarly(int errorCode);
};
