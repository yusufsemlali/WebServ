#include "ClientConnection.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

#include "ClientConnection.hpp"
#include "RequestHandler.hpp"
#include "utiles.hpp"

ClientConnection::ClientConnection(int socketFd, const struct sockaddr_in &clientAddr, RequestHandler &handler, const Config::ServerConfig* serverConfig)
    : socketFd(socketFd),
      clientAddress(parseClientAddress(clientAddr)),
      serverConfig(serverConfig),
      handleRequest(handler),
      connected(true),
      keepAlive(false),
      lastActivity(time(NULL)),
      bytesRead(0),
      bytesWritten(0),
      writeOffset(0)
{
}

ClientConnection::~ClientConnection()
{
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
            close();
            return false;
        }
        // bytesReadNow == 0 means client closed connection
        std::cout << "Client closed connection" << std::endl;
        close();
        return false;
    }

    readBuffer.append(buffer, static_cast<size_t>(bytesReadNow));
    bytesRead += bytesReadNow;
    updateLastActivity();

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

    // FIXED: Added debug info for write operations
    std::cout << "Writing data to socket " << socketFd << std::endl;
    std::cout << "Write buffer size: " << writeBuffer.size() << " bytes" << std::endl;
    std::cout << "Write offset: " << writeOffset << std::endl;

    ssize_t bytesWrittenNow = send(socketFd, writeBuffer.data() + writeOffset, writeBuffer.size() - writeOffset, 0);
    if (bytesWrittenNow < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            std::cout << "Socket would block, will retry later" << std::endl;
            return true; // Not an error, just need to try again
        }
        std::cerr << "Error writing to socket: " << strerror(errno) << std::endl;
        close();
        return false;
    }

    bytesWritten += bytesWrittenNow;
    writeOffset += bytesWrittenNow;
    updateLastActivity();

    // std::cout << "Wrote " << bytesWrittenNow << " bytes to socket" << std::endl;

    // Check if we've sent everything
    if (writeOffset >= writeBuffer.size())
    {
        // std::cout << "Complete response sent successfully" << std::endl;
        writeBuffer.clear();
        writeOffset = 0;
        return true;  // Complete response sent
    }

    std::cout << "Partial write, " << (writeBuffer.size() - writeOffset) << " bytes remaining" << std::endl;
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

bool ClientConnection::hasCompleteRequest() const
{
    return readBuffer.find("\r\n\r\n") != std::string::npos;
}

HttpRequest &ClientConnection::getCurrentRequest()
{
    return currentRequest;
}

HttpResponse &ClientConnection::getCurrentResponse()
{
    return currentResponse;
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

// FIXED: This is the critical function that was causing the empty response
bool ClientConnection::processReadBuffer()
{
    if (!hasCompleteRequest())
    {
        std::cout << "Request not complete yet, waiting for more data..." << std::endl;
        return false;
    }

    if (currentRequest.parseRequest(readBuffer))
    {
        currentResponse.reset();
        
        // Handle the request using socket-based server config (source of truth)
        handleRequest.handleRequest(currentRequest, currentResponse, socketFd, serverConfig);
        
        // TEMPORARY: Check if CGI is pending (don't send response immediately)
        if (currentResponse.getStatusCode() == 0) {  // CGI_PENDING marker
            std::cout << "=== CGI PENDING - NOT SENDING RESPONSE YET ===" << std::endl;
            std::cout << "CGI is running asynchronously, response will be sent when ready" << std::endl;
            return true; // Keep connection alive, don't send response yet
        }
        
        // CRITICAL FIX: Convert response to string and put in write buffer
        writeBuffer = currentResponse.toString();
        writeOffset = 0;
        
        std::cout << "=== RESPONSE GENERATED ===" << std::endl;
        std::cout << "Response ready, buffer size: " << writeBuffer.size() << " bytes" << std::endl;
        std::cout << "Response preview:" << std::endl;
        std::cout << writeBuffer.substr(0, 200) << "..." << std::endl;
        std::cout << "=== END RESPONSE PREVIEW ===" << std::endl;
        
        return true; // Request processed, response ready
    }
    else
    {
        std::cerr << "Failed to parse request" << std::endl;
        
        // Generate 400 Bad Request response
        currentResponse.reset();
        currentResponse.setStatus(400, "Bad Request");
        currentResponse.setBody("Invalid HTTP request format");
        currentResponse.setHeader("Content-Type", "text/plain");
        
        writeBuffer = currentResponse.toString();
        writeOffset = 0;
        
        return true;
    }
}

// REMOVED: These old functions are not needed and may cause conflicts
/*
void ClientConnection::serveStaticFile(const std::string &requestPath)
void ClientConnection::serve404()
std::string ClientConnection::getContentType(const std::string &filePath)
*/