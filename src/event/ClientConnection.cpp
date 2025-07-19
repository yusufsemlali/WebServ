#include "ClientConnection.hpp"
#include "utiles.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <errno.h>
#include <string.h>
#include <stdexcept>

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
        if (!isReadyToRead())
        {
                std::cerr << "Connection not ready to read" << std::endl;
                return false;
        }

        char buffer[MAX_BUFFER_SIZE];

        // Read once per epoll event - let epoll handle the scheduling
        ssize_t bytesReadNow = recv(socketFd, buffer, sizeof(buffer), 0);

        if (bytesReadNow <= 0)
        {
                if (bytesReadNow < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                {
                        // No data available right now - this is normal for non-blocking sockets
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

        // Append the data we read
        readBuffer.append(buffer, static_cast<size_t>(bytesReadNow));
        bytesRead += bytesReadNow;
        updateLastActivity(); // Update activity timestamp

        // Check if we have a complete request
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
                std::cerr << "Error writing to socket: " << strerror(errno) << std::endl;
                close();
                return false;
        }

        bytesWritten += bytesWrittenNow;
        writeOffset += bytesWrittenNow;
        updateLastActivity(); // Update activity timestamp

        // Check if we've sent everything
        if (writeOffset >= writeBuffer.size())
        {
                writeBuffer.clear();
                writeOffset = 0;
                return true; // Complete response sent
        }

        return false; // Partial write, need to continue later
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
        // Check for complete HTTP request (ends with \r\n\r\n)
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
        // Check if we have a complete HTTP request
        if (!hasCompleteRequest())
        {
                return false; // Not ready yet, keep accumulating data
        }

        // Parse the request and generate response
        // For now, simple implementation:
        std::cout << "Complete request received: " << readBuffer << std::endl;

        // Generate a proper HTTP response
        writeBuffer = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";

        return true; // Request processed, response ready
}
