#ifndef REQUEST_BODY_BUFFER_HPP
#define REQUEST_BODY_BUFFER_HPP

#include <string>
#include <ctime>

class RequestBodyBuffer
{
public:
    RequestBodyBuffer();
    ~RequestBodyBuffer();

    static const size_t CLIENT_BODY_BUFFER_SIZE = 16384;  // 16KB threshold
    
    bool initialize(size_t expectedBodySize, int clientFd);
    
    bool writeChunk(const char* data, size_t size);
    
    bool isComplete() const;
    
    bool isBufferingToDisk() const;
    size_t getBytesReceived() const;
    size_t getExpectedSize() const;
    
    std::string getBody() const;
    std::string getTempFilePath() const;
    
    void reset();
    
private:
    size_t expectedBodySize;
    size_t bytesReceived;
    bool bufferingToDisk;
    
    std::string memoryBuffer;
    
    std::string tempFilePath;
    int tempFileFd;
    
    bool shouldBufferToDisk(size_t contentLength) const;
    bool createTempFile(int clientFd);
    bool writeChunkToDisk(const char* data, size_t size);
    bool writeChunkToMemory(const char* data, size_t size);
    void cleanupTempFile();
    std::string readBodyFromDisk() const;
};

#endif // REQUEST_BODY_BUFFER_HPP
