#pragma once

#include <map>
#include <string>
#include <vector>

class HttpRequest
{
       public:
	HttpRequest();
	~HttpRequest();

	// Request parsing
	bool parseRequest(const std::string &rawRequest);
	bool parseHeadersOnly(const std::string &rawRequest);  // Parse only headers (no body)
	bool isComplete() const;
	void reset();

	// Getters
	const std::string &getMethod() const;
	const std::string &getUri() const;
	const std::string &getVersion() const;
	const std::string &getQuery() const;
	const std::map<std::string, std::string> &getHeaders() const;
	const std::string &getBody() const;
	
	// Query parameter operations
	std::string getQueryParam(const std::string &key) const;
	bool hasQueryParam(const std::string &key) const;
	const std::map<std::string, std::string> &getQueryParams() const;

	// Header operations
	std::string getHeader(const std::string &name) const;
	bool hasHeader(const std::string &name) const;

	// Content operations
	size_t getContentLength() const;
	std::string getContentType() const;

	// Validation
	bool isValidMethod() const;
	bool isValidVersion() const;
	bool isValidUri() const;

       private:
	std::string method;
	std::string uri;
	std::string version;
	std::string query;
	std::map<std::string, std::string> headers;
	std::map<std::string, std::string> queryParams;
	std::string body;

	bool requestComplete;
	bool headersParsed;

	// Parsing helpers
	bool parseRequestLine(const std::string &rawRequest);
	bool parseHeaders(const std::string &rawRequest);
	bool parseBody(const std::string &rawRequest);
	bool parseHeader(const std::string &line);
	void parseUri(const std::string &fullUri);
	void parseQueryString(const std::string &queryString);
	std::string urlDecodeQuery(const std::string &str) const;
	std::string toLower(const std::string &str) const;
};
