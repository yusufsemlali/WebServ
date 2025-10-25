#include <csignal>
#include <iostream>
#include <stdexcept>

#include "Core.hpp"
#ifdef DEBUG
#include "debug.hpp"
#endif

int main(void)
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGQUIT, signalHandler);
    signal(SIGPIPE, SIG_IGN);

    try
    {
        const std::string configFilePath = "configs/default.conf";
        Config config = parse(configFilePath);

#ifdef VERBOSE_LOGGING
        std::cout << "Configuration file parsed successfully." << std::endl;
#endif

        config.validateAndParseConfig();

#ifdef DEBUG
        printRawConfig(config);
        std::cout << "\n=== PARSED/VALIDATED CONFIGURATION ===" << std::endl;
        printParsedConfig(config);
        printValidationSummary(config);
        printServerStartup(config);
#endif

        HttpServer server(config);
        g_server = &server;

#ifdef VERBOSE_LOGGING
        std::cout << "Starting HTTP server..." << std::endl;
#endif
        server.run();

        g_server = NULL;

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
