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

#include "utiles.hpp"

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

    ssize_t bytesWrittenNow = send(socketFd, writeBuffer.data() + writeOffset, writeBuffer.size() - writeOffset, 0);
    if (bytesWrittenNow < 0)
    {
        std::cerr << "Error writing to socket: " << strerror(errno) << std::endl;
        close();
        return false;
    }

    bytesWritten += bytesWrittenNow;
    writeOffset += bytesWrittenNow;
    updateLastActivity();  // Update activity timestamp

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
    if (!hasCompleteRequest())
    {
        return false;
    }

    std::cout << "Complete request received: " << readBuffer << std::endl;
    if (currentRequest.parseRequest(readBuffer))
    {
        currentResponse.reset();
        currentResponse.setStatus(200, "OK");
        currentResponse.setHeader("Content-Type", "text/html");

        // Serve static file or handle request
        serveStaticFile(currentRequest.getUri());
    }
    else
    {
        // Handle parse error (e.g., set error response)
        currentResponse.setStatus(400, "Bad Request");
    }

    return true;  // Request processed, response ready
}

void ClientConnection::serveStaticFile(const std::string &requestPath)
{
    // Default document root (now project-local)
    std::string documentRoot = "./root";

    // Build full file path
    std::string filePath = documentRoot + requestPath;

    // If path ends with '/', serve index.html
    if (requestPath == "/" || (!requestPath.empty() && requestPath[requestPath.length() - 1] == '/'))
    {
        if (!requestPath.empty() && requestPath[requestPath.length() - 1] != '/')
            filePath += "/";
        filePath += "index.html";
    }

    std::cout << "Attempting to serve file: " << filePath << std::endl;

    // Try to read the file
    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (file.is_open())
    {
        // Read file content
        std::ostringstream oss;
        oss << file.rdbuf();
        std::string content = oss.str();
        file.close();

        // Determine content type
        std::string contentType = getContentType(filePath);

        // Build HTTP response
        std::ostringstream response;
        response << "HTTP/1.0 200 OK\r\n";
        response << "Content-Type: " << contentType << "\r\n";
        response << "Content-Length: " << content.length() << "\r\n";
        response << "Server: WebServ/1.0\r\n";
        response << "\r\n";
        response << content;

        writeBuffer = response.str();
    }
    else
    {
        // File not found - serve 404
        serve404();
    }
}

std::string ClientConnection::getContentType(const std::string &filePath)
{
    // Get file extension
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos)
    {
        return "text/plain";
    }

    std::string extension = filePath.substr(dotPos + 1);

    // Convert to lowercase
    for (size_t i = 0; i < extension.length(); ++i)
    {
        extension[i] = std::tolower(extension[i]);
    }

    // Map common extensions to MIME types
    if (extension == "html" || extension == "htm")
        return "text/html";
    else if (extension == "css")
        return "text/css";
    else if (extension == "js")
        return "application/javascript";
    else if (extension == "json")
        return "application/json";
    else if (extension == "png")
        return "image/png";
    else if (extension == "jpg" || extension == "jpeg")
        return "image/jpeg";
    else if (extension == "gif")
        return "image/gif";
    else if (extension == "ico")
        return "image/x-icon";
    else if (extension == "txt")
        return "text/plain";
    else
        return "application/octet-stream";
}

void ClientConnection::serve404()
{
    std::string content =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>404 Not Found</title></head>\n"
        "<body>\n"
        "<h1>404 Not Found</h1>\n"
        "<p>The requested file was not found on this server.</p>\n"
        "<hr>\n"
        "<p><em>WebServ/1.0</em></p>\n"
        "</body>\n"
        "</html>\n";

    std::ostringstream response;
    response << "HTTP/1.0 404 Not Found\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "Server: WebServ/1.0\r\n";
    response << "\r\n";
    response << content;

    writeBuffer = response.str();
}
