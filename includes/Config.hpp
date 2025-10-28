#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

class Config
{
   public:
    struct ListenConfig
    {
        std::string host;
        std::string port;
        bool isDefault;
    };

    struct ErrorPageConfig
    {
        int errorCode;
        std::string filePath;
    };

    struct LocationConfig
    {
        std::string path;
        std::map<std::string, std::vector<std::string> > directives;

        std::set<std::string> allowedMethods;
        std::string root;
        std::string index;
        bool autoindex;
        size_t clientMaxBodySize;
        std::string cgiPass;
        std::string returnUrl;
        int returnStatusCode;
        std::vector<ErrorPageConfig> errorPages;
    };

    struct ServerConfig
    {
        std::map<std::string, std::vector<std::string> > directives;
        std::vector<LocationConfig> locations;

        std::vector<ListenConfig> listenConfigs;
        std::vector<std::string> serverNames;
        std::string root;
        std::string index;
        std::set<std::string> allowedMethods;
        bool autoindex;
        size_t clientMaxBodySize;
        std::vector<ErrorPageConfig> errorPages;
    };

    std::vector<ServerConfig> servers;

    Config();
    ~Config();

    void validateAndParseConfig();

   private:
    void parseServerConfig(ServerConfig &server);
    void parseLocationConfig(LocationConfig &location);

    void validateDirectiveValues(const std::string &directive, const std::vector<std::string> &values);
    void validateHttpMethods(const std::vector<std::string> &methods);
    void validateListenValue(const std::string &value);
    void validatePortNumber(int port);
    void validateHostname(const std::string &host);
    void validateFilePath(const std::string &path);
    void validateServerName(const std::string &serverName);
    void validateErrorCode(int errorCode);
    void validateClientSize(const std::string &value);
    void validateAutoindexValue(const std::string &value);
    void validateReturnValue(const std::string &value);
    void validateCgiPath(const std::string &path);
    void validateDuplicateListenAddresses(const std::vector<ListenConfig> &listenConfigs);
    void validateLocationRedirects(const ServerConfig &server);

    ListenConfig parseListenDirective(const std::string &value);
    std::set<std::string> parseMethodsDirective(const std::vector<std::string> &values);
    size_t parseClientSizeDirective(const std::string &value);
    bool parseAutoindexDirective(const std::string &value);
    std::vector<ErrorPageConfig> parseErrorPageDirective(const std::vector<std::string> &values);

    bool isValidHttpMethod(const std::string &method);
    bool isValidErrorCode(int code);
    bool isValidPort(int port);
    bool isValidHostname(const std::string &host);
    bool isValidFilePath(const std::string &path);
    bool isValidUrl(const std::string &url);
    bool isValidReturnValue(const std::string &value);

    void throwValidationError(const std::string &directive, const std::string &value, const std::string &reason);
};
