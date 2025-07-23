#pragma once

#include <map>
#include <string>

class HttpResponse
{
 public:
  HttpResponse();
  ~HttpResponse();

  // Response building
  void setStatus(int code, const std::string &message = "");
  void setHeader(const std::string &name, const std::string &value);
  void setBody(const std::string &content);
  void setBodyFromFile(const std::string &filePath);

  // Content operations
  void appendBody(const std::string &content);
  void clearBody();

  // Response generation
  std::string toString() const;
  const std::string &getBody() const;

  // Status operations
  int getStatusCode() const;
  const std::string &getStatusMessage() const;

  // Header operations
  std::string getHeader(const std::string &name) const;
  bool hasHeader(const std::string &name) const;
  void removeHeader(const std::string &name);

  // Utility methods
  void reset();
  bool isReady() const;

  // Static helper methods
  static std::string getDefaultStatusMessage(int code);
  static void setServerName(const std::string &name);

 private:
  int statusCode;
  std::string statusMessage;
  std::map<std::string, std::string> headers;
  std::string body;

  static std::string serverName;

  // Helper methods
  void setDefaultHeaders();
  std::string formatHeaders() const;
  std::string toLower(const std::string &str) const;
};
