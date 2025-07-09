#include "debug.hpp"
#include <iostream>
#include <set>

void printRawConfig(const Config &config, int indent)
{
        for (std::vector<Config::ServerConfig>::const_iterator serverIt = config.servers.begin(); serverIt != config.servers.end(); ++serverIt)
        {
                for (int i = 0; i < indent; ++i)
                        std::cout << "  ";
                std::cout << "Server Directives:" << std::endl;

                for (std::map<std::string, std::vector<std::string> >::const_iterator dirIt = serverIt->directives.begin(); dirIt != serverIt->directives.end(); ++dirIt)
                {
                        for (int i = 0; i < indent + 1; ++i)
                                std::cout << "  ";
                        std::cout << dirIt->first << ": ";
                        for (std::vector<std::string>::const_iterator valIt = dirIt->second.begin(); valIt != dirIt->second.end(); ++valIt)
                        {
                                std::cout << *valIt << " ";
                        }
                        std::cout << std::endl;
                }

                for (std::vector<Config::LocationConfig>::const_iterator locIt = serverIt->locations.begin(); locIt != serverIt->locations.end(); ++locIt)
                {
                        for (int i = 0; i < indent; ++i)
                                std::cout << "  ";
                        std::cout << "Location: " << locIt->path << std::endl;

                        for (std::map<std::string, std::vector<std::string> >::const_iterator dirIt = locIt->directives.begin(); dirIt != locIt->directives.end(); ++dirIt)
                        {
                                for (int i = 0; i < indent + 1; ++i)
                                        std::cout << "  ";
                                std::cout << dirIt->first << ": ";
                                for (std::vector<std::string>::const_iterator valIt = dirIt->second.begin(); valIt != dirIt->second.end(); ++valIt)
                                {
                                        std::cout << *valIt << " ";
                                }
                                std::cout << std::endl;
                        }
                }
        }
}

void printParsedConfig(const Config &config)
{
        for (size_t i = 0; i < config.servers.size(); ++i)
        {
                const Config::ServerConfig &server = config.servers[i];
                std::cout << "Server " << (i + 1) << ":" << std::endl;

                // Print listen configs
                std::cout << "  Listen: ";
                for (size_t j = 0; j < server.listenConfigs.size(); ++j)
                {
                        const Config::ListenConfig &listen = server.listenConfigs[j];
                        std::cout << listen.host << ":" << listen.port << " ";
                }
                std::cout << std::endl;

                // Print server names
                if (!server.serverNames.empty())
                {
                        std::cout << "  Server Names: ";
                        for (size_t j = 0; j < server.serverNames.size(); ++j)
                        {
                                std::cout << server.serverNames[j] << " ";
                        }
                        std::cout << std::endl;
                }

                // Print other server config
                if (!server.root.empty())
                        std::cout << "  Root: " << server.root << std::endl;
                if (!server.index.empty())
                        std::cout << "  Index: " << server.index << std::endl;

                if (!server.allowedMethods.empty())
                {
                        std::cout << "  Methods: ";
                        for (std::set<std::string>::const_iterator it = server.allowedMethods.begin();
                             it != server.allowedMethods.end(); ++it)
                        {
                                std::cout << *it << " ";
                        }
                        std::cout << std::endl;
                }

                std::cout << "  Autoindex: " << (server.autoindex ? "on" : "off") << std::endl;
                std::cout << "  Client Max Body Size: " << server.clientMaxBodySize << " bytes" << std::endl;

                // Print error pages
                if (!server.errorPages.empty())
                {
                        std::cout << "  Error Pages: ";
                        for (size_t j = 0; j < server.errorPages.size(); ++j)
                        {
                                const Config::ErrorPageConfig &errorPage = server.errorPages[j];
                                std::cout << errorPage.errorCode << "=>" << errorPage.filePath << " ";
                        }
                        std::cout << std::endl;
                }

                // Print locations
                for (size_t j = 0; j < server.locations.size(); ++j)
                {
                        const Config::LocationConfig &location = server.locations[j];
                        std::cout << "  Location '" << location.path << "':" << std::endl;

                        if (!location.allowedMethods.empty())
                        {
                                std::cout << "    Methods: ";
                                for (std::set<std::string>::const_iterator it = location.allowedMethods.begin();
                                     it != location.allowedMethods.end(); ++it)
                                {
                                        std::cout << *it << " ";
                                }
                                std::cout << std::endl;
                        }

                        if (!location.root.empty())
                                std::cout << "    Root: " << location.root << std::endl;
                        if (!location.index.empty())
                                std::cout << "    Index: " << location.index << std::endl;
                        if (!location.cgiPass.empty())
                                std::cout << "    CGI Pass: " << location.cgiPass << std::endl;
                        if (!location.returnUrl.empty())
                                std::cout << "    Return: " << location.returnUrl << std::endl;

                        std::cout << "    Autoindex: " << (location.autoindex ? "on" : "off") << std::endl;

                        if (location.clientMaxBodySize > 0)
                                std::cout << "    Client Max Body Size: " << location.clientMaxBodySize << " bytes" << std::endl;

                        if (!location.errorPages.empty())
                        {
                                std::cout << "    Error Pages: ";
                                for (size_t k = 0; k < location.errorPages.size(); ++k)
                                {
                                        const Config::ErrorPageConfig &errorPage = location.errorPages[k];
                                        std::cout << errorPage.errorCode << "=>" << errorPage.filePath << " ";
                                }
                                std::cout << std::endl;
                        }
                }
                std::cout << std::endl;
        }
}

void printConfigSummary(const Config &config)
{
        std::cout << "=== CONFIGURATION SUMMARY ===" << std::endl;
        std::cout << "Total servers: " << config.servers.size() << std::endl;

        for (size_t i = 0; i < config.servers.size(); ++i)
        {
                const Config::ServerConfig &server = config.servers[i];
                std::cout << "Server " << (i + 1) << ": ";

                for (size_t j = 0; j < server.listenConfigs.size(); ++j)
                {
                        const Config::ListenConfig &listen = server.listenConfigs[j];
                        std::cout << listen.host << ":" << listen.port;
                        if (j < server.listenConfigs.size() - 1)
                                std::cout << ", ";
                }

                std::cout << " (" << server.locations.size() << " locations)" << std::endl;
        }
        std::cout << std::endl;
}

void printServerStartup(const Config &config)
{
        std::cout << "=== STARTING WEBSERVER ===" << std::endl;
        printConfigSummary(config);
        printListenAddresses(config);
}

void printListenAddresses(const Config &config)
{
        std::cout << "=== LISTEN ADDRESSES ===" << std::endl;

        for (size_t i = 0; i < config.servers.size(); ++i)
        {
                const Config::ServerConfig &server = config.servers[i];

                for (size_t j = 0; j < server.listenConfigs.size(); ++j)
                {
                        const Config::ListenConfig &listen = server.listenConfigs[j];
                        std::cout << "Binding to " << listen.host << ":" << listen.port;

                        if (!server.serverNames.empty())
                        {
                                std::cout << " (server_name: ";
                                for (size_t k = 0; k < server.serverNames.size(); ++k)
                                {
                                        std::cout << server.serverNames[k];
                                        if (k < server.serverNames.size() - 1)
                                                std::cout << ", ";
                                }
                                std::cout << ")";
                        }
                        std::cout << std::endl;
                }
        }
        std::cout << std::endl;
}

void validateConfigCompliance(const Config &config)
{
        std::cout << "=== CONFIGURATION VALIDATION ===" << std::endl;

        for (size_t i = 0; i < config.servers.size(); ++i)
        {
                const Config::ServerConfig &server = config.servers[i];
                std::cout << "Server " << (i + 1) << " validation:" << std::endl;

                // Check if server has listen directive
                if (server.listenConfigs.empty())
                {
                        std::cout << "  WARNING: No listen directive found, using default" << std::endl;
                }

                // Check port numbers
                for (size_t j = 0; j < server.listenConfigs.size(); ++j)
                {
                        const Config::ListenConfig &listen = server.listenConfigs[j];
                        if (listen.port < 1 || listen.port > 65535)
                        {
                                std::cout << "  ERROR: Invalid port number " << listen.port << std::endl;
                        }
                        else if (listen.port < 1024)
                        {
                                std::cout << "  WARNING: Port " << listen.port << " requires root privileges" << std::endl;
                        }
                }

                // Check root directory
                if (server.root.empty())
                {
                        std::cout << "  WARNING: No root directory specified" << std::endl;
                }

                // Check locations
                for (size_t j = 0; j < server.locations.size(); ++j)
                {
                        const Config::LocationConfig &location = server.locations[j];
                        if (location.path.empty())
                        {
                                std::cout << "  ERROR: Location with empty path found" << std::endl;
                        }
                }
        }
        std::cout << std::endl;
}

bool checkPortAvailability(int port)
{
        // This is a simple check - in a real implementation you'd try to bind to the port
        if (port < 1 || port > 65535)
                return false;
        return true;
}

void printValidationSummary(const Config &config)
{
        std::cout << "=== VALIDATION SUMMARY ===" << std::endl;

        int totalServers = config.servers.size();
        int totalLocations = 0;
        int totalListeners = 0;

        for (size_t i = 0; i < config.servers.size(); ++i)
        {
                const Config::ServerConfig &server = config.servers[i];
                totalLocations += server.locations.size();
                totalListeners += server.listenConfigs.size();
        }

        std::cout << "Servers: " << totalServers << std::endl;
        std::cout << "Listeners: " << totalListeners << std::endl;
        std::cout << "Locations: " << totalLocations << std::endl;
        std::cout << "Configuration appears valid" << std::endl;
        std::cout << std::endl;
}
