#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>

class Config
{
public:
        // Parsed/validated structures
        struct ListenConfig
        {
                std::string host;
                int port;
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
                // Raw directives (from parser)
                std::map<std::string, std::vector<std::string> > directives;

                // Parsed/validated data
                std::set<std::string> allowedMethods;
                std::string root;
                std::string index;
                bool autoindex;
                size_t clientMaxBodySize;
                std::string cgiPass;
                std::string returnUrl;
                std::vector<ErrorPageConfig> errorPages;
        };

        struct ServerConfig
        {
                // Raw directives (from parser)
                std::map<std::string, std::vector<std::string> > directives;
                std::vector<LocationConfig> locations;

                // Parsed/validated data
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

        Config() {}
        ~Config() {}

        // Methods to parse/validate raw directives into typed structures
        void validateAndParseConfig();

private:
        void parseServerConfig(ServerConfig &server);
        void parseLocationConfig(LocationConfig &location);

        // Helper parsing methods
        ListenConfig parseListenDirective(const std::string &value);
        std::set<std::string> parseMethodsDirective(const std::vector<std::string> &values);
        size_t parseClientSizeDirective(const std::string &value);
        bool parseAutoindexDirective(const std::string &value);
        std::vector<ErrorPageConfig> parseErrorPageDirective(const std::vector<std::string> &values);
};
