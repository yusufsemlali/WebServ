#include <csignal>
#include <cstdlib>
#include <iostream>

#include "core.hpp"

// HttpServer *g_server = NULL;

// static volatile bool shutdown_in_progress = false;

volatile sig_atomic_t shutdown_requested = 0;
volatile sig_atomic_t received_signal = 0;

void signalHandler(int signal)
{
    // Prevent multiple signal handling
    received_signal = 128 + signal;

    if (shutdown_requested)
    {
        std::cout << "\nForced shutdown..." << std::endl;
        exit(received_signal);
    }

    shutdown_requested = 1;

    std::cout << std::endl;  // Add newline after ^C

    switch (signal)
    {
        case SIGINT:
            std::cout << "Received SIGINT (Ctrl+C). Shutting down gracefully..." << std::endl;
            break;
        case SIGTERM:
            std::cout << "Received SIGTERM. Shutting down gracefully..." << std::endl;
            break;
        case SIGQUIT:
            std::cout << "Received SIGQUIT. Shutting down gracefully..." << std::endl;
            break;
        default:
            std::cout << "Received signal " << signal << ". Shutting down gracefully..." << std::endl;
            break;
    }
}
