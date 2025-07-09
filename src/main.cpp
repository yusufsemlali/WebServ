#include "core.hpp"
#include <iostream>
#include <stdexcept>

void printConfigTree(const Config &config, int indent = 0)
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

        // TODO: Add your web server's startup logic here, using the 'serverConfig' object.
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
