#include "core.hpp"
#include <iostream>
#include <stdexcept>

void printConfigTree(const Config &config, int indent = 0);
void printParsedConfig(const Config &config);

void printConfigTree(const Config &config, int indent)
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

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <path_to_config_file>" << std::endl;
        return 1;
    }

    std::string configFilePath = argv[1];

    try
    {
        Config serverConfig = parse(configFilePath);
        std::cout << "Configuration file parsed successfully." << std::endl;
        printConfigTree(serverConfig);

        // Parse and validate the configuration
        serverConfig.validateAndParseConfig();
        std::cout << "\n=== PARSED/VALIDATED CONFIGURATION ===" << std::endl;
        printParsedConfig(serverConfig);

        // TODO: Add your web server's startup logic here, using the 'serverConfig' object.
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
