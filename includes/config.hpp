#pragma once

#include <string>
#include <vector>
#include <map>

class Config
{
public:
        struct LocationConfig
        {
                std::string path;
                std::map<std::string, std::vector<std::string> > directives;
        };

        struct ServerConfig
        {
                std::map<std::string, std::vector<std::string> > directives;
                std::vector<LocationConfig> locations;
        };
        std::vector<ServerConfig> servers;

        Config() {}  
        ~Config() {} 
};
