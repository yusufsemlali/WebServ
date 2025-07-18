#include "ClientConnection.hpp"
#include "utiles.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <errno.h>
#include <string.h>

ClientConnection::ClientConnection(int socketFd, const struct sockaddr_in &clientAddr)
    : socketFd(socketFd),
      clientAddress(parseClientAddress(clientAddr)),
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
        // TODO - Implement logic to read data from the client
        return false;
}

bool ClientConnection::writeData()
{
        // TODO - Implement logic to write data to the client
        return true;
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
        // TODO - Implement logic to check if the current request is complete
        return false;
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
