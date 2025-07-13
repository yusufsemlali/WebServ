#include "core.hpp"
#include <iostream>
#include <stdexcept>
#ifdef DEBUG
#include "debug.hpp"
#endif

int main(void)
{

    try
    {
        const std::string configFilePath = "configs/default.conf";
        Config config = parse(configFilePath);
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
        HttpServer server(config);
        return server.start();

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
