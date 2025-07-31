#include <csignal>
#include <iostream>

#include "Config.hpp"
#include "HttpServer.hpp"
#include "core.hpp"
#include "parser.hpp"

#ifdef DEBUG
#include "debug.hpp"
#endif

int main(void)
{
    // handle signal interupts
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGQUIT, signalHandler);
    signal(SIGPIPE, SIG_IGN);  // Ignore SIGPIPE to prevent crashes on broken pipes

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
        if (shutdown_requested)
        {
            std::cout << "Server shutdown complete." << std::endl;
            return received_signal;
        }
        {
            HttpServer server(config);

            std::cout << "Starting HTTP server..." << std::endl;
            server.run();
        }
        if (shutdown_requested)
        {
            std::cout << "Server shutdown complete." << std::endl;
            return received_signal;
        }
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
