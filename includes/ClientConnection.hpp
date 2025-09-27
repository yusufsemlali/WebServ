#pragma once

#include <ctime>
#include <string>

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "RequestHandler.hpp"
#include "AsyncOperation.hpp"

// Connection state machine
enum ConnectionState {
    READING_REQUEST,     // Reading HTTP request from client
    PROCESSING_REQUEST,  // Request complete, being processed
    WAITING_ASYNC,       // Waiting for async operation (CGI, etc.)
    WRITING_RESPONSE,    // Writing response to client
    KEEP_ALIVE,          // Waiting for next request on keep-alive connection
    CLOSING              // Connection being closed
};

// Request processing context
struct RequestContext {
    HttpRequest request;
    HttpResponse response;
    ConnectionState state;
    
    // For async operations
    AsyncOperation* pendingOperation;
    
    // Timing
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
    // Statistics
    size_t getBytesRead() const;
    size_t getBytesWritten() const;
    
    // State machine management
    ConnectionState getState() const;
    void setState(ConnectionState newState);
    
    // Async operation support
    void setPendingOperation(AsyncOperation* operation);
    void completePendingOperation();
    bool hasPendingOperation() const;
    AsyncOperation* getPendingOperation() const;
    
    // State queries for event loop
    bool canRead() const;
    bool canWrite() const;
    bool isReadyForCleanup() const;

   private:
    int socketFd;
    std::string clientAddress;

    std::string readBuffer;
    std::string writeBuffer;

    RequestContext context;  // Contains request, response, and state
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
    void parseCgiOutput(const std::string& output);  // Parse CGI output into headers/body
};
