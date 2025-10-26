#include <csignal>
#include <cstdlib>
#include <iostream>

#include "Core.hpp"

HttpServer *g_server = NULL;

static volatile bool shutdown_in_progress = false;

void signalHandler(int signal)
{
  // Prevent multiple signal handling
  if (shutdown_in_progress)
  {
    std::cout << "\nForced shutdown..." << std::endl;
    exit(1);
  }

  shutdown_in_progress = true;
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

  // Safely stop the server if it exists
  if (g_server != NULL)
  {
    std::cout << "Stopping HTTP server..." << std::endl;
    try
    {
      g_server->stop();
      std::cout << "Server stopped successfully." << std::endl;
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error during server shutdown: " << e.what() << std::endl;
    }
  }
  std::cout << "Server shutdown complete." << std::endl;
}
