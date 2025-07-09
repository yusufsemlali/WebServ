#include "core.hpp"
#include <iostream>
#include <stdexcept>
#ifdef DEBUG
#include "debug.hpp"
#endif

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    try
    {
        Config config = parse(argv[1]);
        std::cout << "Configuration file parsed successfully." << std::endl;

        // Parse and validate the configuration
        config.validateAndParseConfig();

#ifdef DEBUG
        printRawConfig(config);
        std::cout << "\n=== PARSED/VALIDATED CONFIGURATION ===" << std::endl;
        printParsedConfig(config);
        printValidationSummary(config);
        printServerStartup(config);
#else
        std::cout << "Server configuration loaded. Ready to start." << std::endl;
#endif

        // Here you would start your actual web server with the config
        // WebServer server(config);
        // return server.run();

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
