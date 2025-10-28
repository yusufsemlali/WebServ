#include "RequestBodyBuffer.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>

RequestBodyBuffer::RequestBodyBuffer()
    : expectedBodySize(0),
      bytesReceived(0),
      bufferingToDisk(false),
      tempFileFd(-1)
{
}

RequestBodyBuffer::~RequestBodyBuffer()
{
    cleanupTempFile();
}

bool RequestBodyBuffer::shouldBufferToDisk(size_t contentLength) const
{
    return contentLength > CLIENT_BODY_BUFFER_SIZE;
}

bool RequestBodyBuffer::initialize(size_t expectedSize, int clientFd)
{
    reset();
    
    expectedBodySize = expectedSize;
    bufferingToDisk = (expectedSize > CLIENT_BODY_BUFFER_SIZE);
    
    if (bufferingToDisk)
    {
        return createTempFile(clientFd);
    }
    
    memoryBuffer.reserve(expectedSize);
    return true;
}

bool RequestBodyBuffer::createTempFile(int clientFd)
{
    std::ostringstream oss;
    oss << "/tmp/webserv_body_" << clientFd << "_" << time(NULL);
    tempFilePath = oss.str();
    
    tempFileFd = open(tempFilePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (tempFileFd < 0)
    {
        std::cerr << "RequestBodyBuffer: Failed to create temp file: " << tempFilePath << std::endl;
        return false;
    }
    
    return true;
}

bool RequestBodyBuffer::writeChunk(const char* data, size_t size)
{
    if (bufferingToDisk)
    {
        return writeChunkToDisk(data, size);
    }
    else
    {
        return writeChunkToMemory(data, size);
    }
}

bool RequestBodyBuffer::writeChunkToDisk(const char* data, size_t size)
{
    if (tempFileFd < 0)
    {
        std::cerr << "RequestBodyBuffer: Temp file not open" << std::endl;
        return false;
    }
    
    ssize_t written = write(tempFileFd, data, size);
    if (written < 0 || static_cast<size_t>(written) != size)
    {
        std::cerr << "RequestBodyBuffer: Failed to write chunk to disk" << std::endl;
        return false;
    }
    
    bytesReceived += size;
    return true;
}

bool RequestBodyBuffer::writeChunkToMemory(const char* data, size_t size)
{
    // Prevent memory overflow
    if (bytesReceived + size > expectedBodySize)
    {
        std::cerr << "RequestBodyBuffer: Chunk exceeds expected body size" << std::endl;
        return false;
    }
    
    memoryBuffer.append(data, size);
    bytesReceived += size;
    return true;
}

bool RequestBodyBuffer::isComplete() const
{
    return bytesReceived >= expectedBodySize;
}

bool RequestBodyBuffer::isBufferingToDisk() const
{
    return bufferingToDisk;
}

size_t RequestBodyBuffer::getBytesReceived() const
{
    return bytesReceived;
}

size_t RequestBodyBuffer::getExpectedSize() const
{
    return expectedBodySize;
}

std::string RequestBodyBuffer::getBody() const
{
    if (bufferingToDisk)
    {
        return readBodyFromDisk();
    }
    else
    {
        return memoryBuffer;
    }
}

std::string RequestBodyBuffer::readBodyFromDisk() const
{
    if (tempFilePath.empty())
    {
        return "";
    }
    
    std::ifstream file(tempFilePath.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "RequestBodyBuffer: Failed to read temp file" << std::endl;
        return "";
    }
    
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

std::string RequestBodyBuffer::getTempFilePath() const
{
    return tempFilePath;
}

void RequestBodyBuffer::cleanupTempFile()
{
    if (tempFileFd >= 0)
    {
        ::close(tempFileFd);
        tempFileFd = -1;
    }
    
    if (!tempFilePath.empty())
    {
        unlink(tempFilePath.c_str());
        tempFilePath.clear();
    }
}

void RequestBodyBuffer::reset()
{
    cleanupTempFile();
    
    expectedBodySize = 0;
    bytesReceived = 0;
    bufferingToDisk = false;
    memoryBuffer.clear();
}
