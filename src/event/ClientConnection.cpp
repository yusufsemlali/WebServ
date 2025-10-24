#include "ClientConnection.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include "RequestHandler.hpp"
#include "utiles.hpp"

ClientConnection::ClientConnection(int socketFd, const struct sockaddr_in &clientAddr, RequestHandler &handler)
    : socketFd(socketFd),
      clientAddress(parseClientAddress(clientAddr)),
      handleRequest(handler),
      connected(true),
      keepAlive(false),
      lastActivity(time(NULL)),
      bytesRead(0),
      bytesWritten(0),
      writeOffset(0)
{
    context.state = READING_REQUEST;
}

ClientConnection::~ClientConnection()
{
    if (context.pendingOperation) {
        context.pendingOperation->cleanup();
        delete context.pendingOperation;
        context.pendingOperation = NULL;
    }
    close();
}

bool ClientConnection::readData()
{
    char buffer[MAX_BUFFER_SIZE];

    ssize_t bytesReadNow = recv(socketFd, buffer, sizeof(buffer), 0);

    if (bytesReadNow <= 0)
    {
        if (bytesReadNow < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            return true;
        }
        if (bytesReadNow < 0)
        {
            std::cerr << "Error reading from socket: " << strerror(errno) << std::endl;
            return false;
        }
        // bytesReadNow == 0 means client closed connection
        std::cout << "Client closed connection" << std::endl;
        return false;
    }

    readBuffer.append(buffer, static_cast<size_t>(bytesReadNow));
    bytesRead += bytesReadNow;
    updateLastActivity();

#ifdef VERBOSE_LOGGING
    // ADDED: Print raw request data as it comes in
    std::cout << "=== RAW REQUEST DATA RECEIVED ===" << std::endl;
    std::cout << readBuffer << std::endl;
    std::cout << "=== END RAW REQUEST DATA ===" << std::endl;
#endif

    if (processReadBuffer())
    {
        readBuffer.clear();
    }

    return true;
}

bool ClientConnection::writeData()
{
    if (!isReadyToWrite())
    {
        std::cerr << "Connection not ready to write" << std::endl;
        return false;
    }

    ssize_t bytesWrittenNow = send(socketFd, writeBuffer.data() + writeOffset, writeBuffer.size() - writeOffset, 0);
    if (bytesWrittenNow < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return true;
        }
        std::cerr << "Error writing to socket: " << strerror(errno) << std::endl;
        return false;
    }

    bytesWritten += bytesWrittenNow;
    writeOffset += bytesWrittenNow;
    updateLastActivity();

    // Check if we've sent everything
    if (writeOffset >= writeBuffer.size())
    {
        writeBuffer.clear();
        writeOffset = 0;
        return true;  // Complete response sent
    }


    return false;  // Partial write, need to continue later
}

void ClientConnection::close()
{
    if (socketFd >= 0)
    {
        if (::close(socketFd) < 0)
        {
            std::cerr << "Failed to close socket: " << strerror(errno) << std::endl;
        }
        socketFd = -1;
    }
    connected = false;
}

bool ClientConnection::isConnected() const
{
    return connected;
}

void ClientConnection::setServerFd(int serverFd)
{
    this->serverFd = serverFd;
}

int ClientConnection::getServerFd() const
{
    return serverFd;
}

bool ClientConnection::hasCompleteRequest() const
{
    // Check for complete HTTP request (ends with \r\n\r\n)
    return readBuffer.find("\r\n\r\n") != std::string::npos;
}

HttpRequest &ClientConnection::getCurrentRequest()
{
    return context.request;
}

HttpResponse &ClientConnection::getCurrentResponse()
{
    return context.response;
}

bool ClientConnection::isKeepAlive() const
{
    return keepAlive;
}

void ClientConnection::setKeepAlive(bool keepAlive)
{
    this->keepAlive = keepAlive;
}

bool ClientConnection::isReadyToWrite() const
{
    return !writeBuffer.empty();
}

bool ClientConnection::isReadyToRead() const
{
    return connected;
}

void ClientConnection::updateLastActivity()
{
    lastActivity = time(NULL);
}

bool ClientConnection::isTimedOut(int timeoutSeconds) const
{
    return (time(NULL) - lastActivity) > timeoutSeconds;
}

int ClientConnection::getSocketFd() const
{
    return socketFd;
}

std::string ClientConnection::getClientAddress() const
{
    return clientAddress;
}

void ClientConnection::clearBuffers()
{
    readBuffer.clear();
    writeBuffer.clear();
    writeOffset = 0;
}

size_t ClientConnection::getBytesRead() const
{
    return bytesRead;
}

size_t ClientConnection::getBytesWritten() const
{
    return bytesWritten;
}

bool ClientConnection::processReadBuffer()
{
    if (!hasCompleteRequest())
    {
        return false;
    }

    if (context.request.parseRequest(readBuffer))
    {
        context.response.reset();
        setState(PROCESSING_REQUEST);
        
        handleRequest.handleRequest(context.request, context.response, this);
        
        if (context.state == WAITING_ASYNC) {
            return true; 
        }
        
        writeBuffer = context.response.toString();
        writeOffset = 0;
        setState(WRITING_RESPONSE);
        
#ifdef VERBOSE_LOGGING
        std::cout << "=== RESPONSE GENERATED ===" << std::endl;
        std::cout << "Response ready, buffer size: " << writeBuffer.size() << " bytes" << std::endl;
        std::cout << "Response preview:" << std::endl;
        std::cout << writeBuffer.substr(0, 200) << "..." << std::endl;
        std::cout << "=== END RESPONSE PREVIEW ===" << std::endl;
#endif
        return true; 
    }
    else
    {
        std::cerr << "Failed to parse request" << std::endl;
        
        // Use RequestHandler to generate proper 400 error page from config
        context.response.reset();
        handleRequest.generateErrorPage(400, context.response, serverFd);
        
        writeBuffer = context.response.toString();
        writeOffset = 0;
        setState(WRITING_RESPONSE);
        
        return true;
    }
}

ConnectionState ClientConnection::getState() const
{
    return context.state;
}

void ClientConnection::setState(ConnectionState newState)
{
#ifdef VERBOSE_LOGGING
    std::cout << "ClientConnection: State transition from " << context.state 
              << " to " << newState << " on socket " << socketFd << std::endl;
#endif
    context.state = newState;
    updateLastActivity();
}

// ===== ASYNC OPERATION SUPPORT =====

void ClientConnection::setPendingOperation(AsyncOperation* operation)
{
    // Clean up any existing operation
    if (context.pendingOperation) {
        context.pendingOperation->cleanup();
        delete context.pendingOperation;
    }
    
    context.pendingOperation = operation;
    setState(WAITING_ASYNC);
#ifdef VERBOSE_LOGGING
    std::cout << "ClientConnection: Set pending async operation on socket " << socketFd << std::endl;
#endif
}

void ClientConnection::completePendingOperation()
{
    if (!context.pendingOperation) {
        std::cerr << "ClientConnection: No pending operation to complete" << std::endl;
        return;
    }
    
    if (context.pendingOperation->isComplete()) {
        if (context.pendingOperation->hasError()) {
            std::cerr << "Async operation failed: " << context.pendingOperation->getError() << std::endl;
            context.response.reset();
            context.response.setStatus(500, "Internal Server Error");
            context.response.setBody("Async operation failed: " + context.pendingOperation->getError());
            context.response.setHeader("Content-Type", "text/plain");
        } else {
            std::string result = context.pendingOperation->getResult();

           
            context.response.reset();
            parseCgiOutput(result);
        }
        
        context.pendingOperation->cleanup();
        delete context.pendingOperation;
        context.pendingOperation = NULL;
        
        writeBuffer = context.response.toString();
        writeOffset = 0;
        setState(WRITING_RESPONSE);
    } else {
        std::cerr << "ClientConnection: Operation not complete, cannot finish!" << std::endl;
    }
}

bool ClientConnection::hasPendingOperation() const
{
    return context.pendingOperation != NULL;
}

AsyncOperation* ClientConnection::getPendingOperation() const
{
    return context.pendingOperation;
}


bool ClientConnection::canRead() const
{
    return connected && (context.state == READING_REQUEST || context.state == KEEP_ALIVE);
}

bool ClientConnection::canWrite() const
{
    return connected && context.state == WRITING_RESPONSE && !writeBuffer.empty();
}

bool ClientConnection::isReadyForCleanup() const
{
    return !connected || context.state == CLOSING;
}

void ClientConnection::parseCgiOutput(const std::string& output)
{
    size_t headerEnd = output.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = output.find("\n\n");
        if (headerEnd != std::string::npos) {
            headerEnd += 2;
        }
    } else {
        headerEnd += 4;
    }
    
    if (headerEnd != std::string::npos) {
        std::string headers = output.substr(0, headerEnd);
        std::string body = output.substr(headerEnd);
        
        std::istringstream headerStream(headers);
        std::string line;
        bool statusSet = false;
        
        while (std::getline(headerStream, line)) {
            if (line.empty() || line == "\r") continue;
            
            if (!line.empty() && line[line.length() - 1] == '\r') {
                line.erase(line.length() - 1);
            }
            
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string name = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);
                
                while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
                    value.erase(0, 1);
                }
                
                if (name == "Status" && !statusSet) {
                    size_t spacePos = value.find(' ');
                    if (spacePos != std::string::npos) {
                        int statusCode = atoi(value.substr(0, spacePos).c_str());
                        std::string statusMessage = value.substr(spacePos + 1);
                        context.response.setStatus(statusCode, statusMessage);
                        statusSet = true;
                    }
                } else {
                    context.response.setHeader(name, value);
                }
            }
        }
        
        if (!statusSet) {
            context.response.setStatus(200, "OK");
        }
        
        context.response.setBody(body);
    } else {
        context.response.setStatus(200, "OK");
        context.response.setHeader("Content-Type", "text/html");
        context.response.setBody(output);
    }
}