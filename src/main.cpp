#include "core.hpp"
#include <iostream>
#include <stdexcept>

int main(int argc, char *argv[])
{
    // 1. Check for the correct number of arguments
    if (argc != 2)
    {
        // Provide usage instructions if the input is incorrect
        std::cerr << "Usage: " << argv[0] << " <path_to_config_file>" << std::endl;
        return 1; // Return a non-zero value to indicate an error
    }

    // 2. Get the file path from the command line
    std::string configFilePath = argv[1];

    try
    {
        // 3. Call your parsing library
        Config serverConfig = parse(configFilePath);

        // If parsing is successful, proceed with server setup
        std::cout << "Configuration file parsed successfully." << std::endl;

        // TODO: Add your web server's startup logic here, using the 'serverConfig' object.
    }
    catch (const std::exception &e)
    {
        // 4. Catch any errors thrown by the parsing library (e.g., file not found, syntax error)
        std::cerr << "Error: " << e.what() << std::endl;
        return 1; // Return an error code
    }

    return 0; // Return 0 to indicate success
}