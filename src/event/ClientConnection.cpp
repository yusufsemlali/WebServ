#include "ClientConnection.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <errno.h>
#include <string.h>

ClientConnection::ClientConnection(int socketFd)
    : socketFd(socketFd), connected(true), keepAlive(false),
      lastActivity(time(NULL)), bytesRead(0), bytesWritten(0), writeOffset(0)
{
        parseClientAddress();
}

ClientConnection::~ClientConnection()
{
        close();
}

void ClientConnection::parseClientAddress()
{
        struct sockaddr_in addr;
        socklen_t addrLen = sizeof(addr);

        if (getpeername(socketFd, (struct sockaddr *)&addr, &addrLen) == 0)
        {
                clientAddress = inet_ntoa(addr.sin_addr);
                clientAddress += ":";

                // Convert port to string manually for C++98 compatibility
                std::ostringstream portStream;
                portStream << ntohs(addr.sin_port);
                clientAddress += portStream.str();
        }
        else
        {
                clientAddress = "unknown";
        }
}

bool ClientConnection::readData()
{
        if (!connected)
                return false;

        char buffer[MAX_BUFFER_SIZE];
        ssize_t bytesReceived = recv(socketFd, buffer, sizeof(buffer), 0);

        if (bytesReceived > 0)
        {
                readBuffer.append(buffer, bytesReceived);
                bytesRead += bytesReceived;
                updateLastActivity();
                return processReadBuffer();
        }
        else if (bytesReceived == 0)
        {
                // Connection closed by client
                connected = false;
                return false;
        }
        else
        {
                // Error occurred
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                {
                        connected = false;
                }
                return false;
        }
}

bool ClientConnection::writeData()
{
        if (!connected || writeBuffer.empty())
                return false;

        size_t remainingBytes = writeBuffer.length() - writeOffset;
        ssize_t bytesSent = send(socketFd, writeBuffer.c_str() + writeOffset, remainingBytes, 0);

        if (bytesSent > 0)
        {
                writeOffset += bytesSent;
                bytesWritten += bytesSent;
                updateLastActivity();

                // If all data sent, clear the buffer
                if (writeOffset >= writeBuffer.length())
                {
                        writeBuffer.clear();
                        writeOffset = 0;
                        return true;
                }
                return false; // More data to send
        }
        else if (bytesSent == 0)
        {
                return false;
        }
        else
        {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                {
                        connected = false;
                }
                return false;
        }
}

void ClientConnection::close()
{
        if (socketFd >= 0)
        {
                ::close(socketFd);
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
        // Check if we have a complete HTTP request
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

bool ClientConnection::processReadBuffer()
{
        // Simple implementation - just check if we have complete request
        // In a full implementation, this would parse the HTTP request
        return hasCompleteRequest();
}
