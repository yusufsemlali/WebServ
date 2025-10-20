#include <csignal>
#include <iostream>
#include <stdexcept>

#include "core.hpp"
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
        std::cout << "Configuration file parsed successfully." << std::endl;

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

        HttpServer server(config);
        g_server = &server;

        std::cout << "Starting HTTP server..." << std::endl;
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
