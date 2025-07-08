#pragma once

#include <string>
#include <vector>
#include <map>

class Config
{
public:
        // A struct to hold the configuration for a single 'location' block.
        struct LocationConfig
        {
                std::string path;
                // Use a map for directives that can have multiple values
                std::map<std::string, std::vector<std::string> > directives;
        };

        // A struct to hold the configuration for a single 'server' block.
        struct ServerConfig
        {
                // Use a map for directives that can have multiple values
                std::map<std::string, std::vector<std::string> > directives;

                // A server can have multiple location blocks.
                std::vector<LocationConfig> locations;
        };

        // The main Config object will hold a list of all server configurations found.
        std::vector<ServerConfig> servers;

        Config() {}  // Default constructor
        ~Config() {} // Default destructor
};
