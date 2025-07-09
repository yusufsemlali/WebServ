#include "config.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

void Config::validateAndParseConfig()
{
        for (size_t i = 0; i < servers.size(); ++i)
        {
                parseServerConfig(servers[i]);
        }
}

void Config::parseServerConfig(ServerConfig &server)
{
        // Parse listen directives
        if (server.directives.find("listen") != server.directives.end())
        {
                const std::vector<std::string> &listenValues = server.directives["listen"];
                for (size_t i = 0; i < listenValues.size(); ++i)
                {
                        server.listenConfigs.push_back(parseListenDirective(listenValues[i]));
                }
        }

        // Parse server_name
        if (server.directives.find("server_name") != server.directives.end())
        {
                server.serverNames = server.directives["server_name"];
        }

        // Parse root
        if (server.directives.find("root") != server.directives.end())
        {
                server.root = server.directives["root"].back(); // Use last value
        }

        // Parse index
        if (server.directives.find("index") != server.directives.end())
        {
                server.index = server.directives["index"].back(); // Use last value
        }

        // Parse methods
        if (server.directives.find("methods") != server.directives.end())
        {
                server.allowedMethods = parseMethodsDirective(server.directives["methods"]);
        }

        // Parse autoindex
        if (server.directives.find("autoindex") != server.directives.end())
        {
                server.autoindex = parseAutoindexDirective(server.directives["autoindex"].back());
        }
        else
        {
                server.autoindex = false; // Default
        }

        // Parse client_size
        if (server.directives.find("client_size") != server.directives.end())
        {
                server.clientMaxBodySize = parseClientSizeDirective(server.directives["client_size"].back());
        }
        else if (server.directives.find("client_max_body_size") != server.directives.end())
        {
                server.clientMaxBodySize = parseClientSizeDirective(server.directives["client_max_body_size"].back());
        }
        else
        {
                server.clientMaxBodySize = 1048576; // Default 1MB
        }

        // Parse error_page
        if (server.directives.find("error_page") != server.directives.end())
        {
                server.errorPages = parseErrorPageDirective(server.directives["error_page"]);
        }

        // Parse location blocks
        for (size_t i = 0; i < server.locations.size(); ++i)
        {
                parseLocationConfig(server.locations[i]);
        }
}

void Config::parseLocationConfig(LocationConfig &location)
{
        // Parse methods
        if (location.directives.find("methods") != location.directives.end())
        {
                location.allowedMethods = parseMethodsDirective(location.directives["methods"]);
        }

        // Parse root
        if (location.directives.find("root") != location.directives.end())
        {
                location.root = location.directives["root"].back();
        }

        // Parse index
        if (location.directives.find("index") != location.directives.end())
        {
                location.index = location.directives["index"].back();
        }

        // Parse autoindex
        if (location.directives.find("autoindex") != location.directives.end())
        {
                location.autoindex = parseAutoindexDirective(location.directives["autoindex"].back());
        }
        else
        {
                location.autoindex = false;
        }

        // Parse client_size
        if (location.directives.find("client_size") != location.directives.end())
        {
                location.clientMaxBodySize = parseClientSizeDirective(location.directives["client_size"].back());
        }
        else
        {
                location.clientMaxBodySize = 0; // Inherit from server
        }

        // Parse cgi_pass
        if (location.directives.find("cgi_pass") != location.directives.end())
        {
                location.cgiPass = location.directives["cgi_pass"].back();
        }

        // Parse return
        if (location.directives.find("return") != location.directives.end())
        {
                location.returnUrl = location.directives["return"].back();
        }

        // Parse error_page
        if (location.directives.find("error_page") != location.directives.end())
        {
                location.errorPages = parseErrorPageDirective(location.directives["error_page"]);
        }
}

Config::ListenConfig Config::parseListenDirective(const std::string &value)
{
        ListenConfig config;
        config.host = "0.0.0.0"; // Default
        config.port = 80;        // Default
        config.isDefault = false;

        if (value.find(':') != std::string::npos)
        {
                // Format: host:port
                size_t colonPos = value.find(':');
                config.host = value.substr(0, colonPos);

                std::string portStr = value.substr(colonPos + 1);
                std::istringstream iss(portStr);
                iss >> config.port;
        }
        else
        {
                // Just port number
                std::istringstream iss(value);
                if (!(iss >> config.port))
                {
                        // Not a number, might be hostname
                        config.host = value;
                        config.port = 80;
                }
        }

        return config;
}

std::set<std::string> Config::parseMethodsDirective(const std::vector<std::string> &values)
{
        std::set<std::string> methods;
        for (size_t i = 0; i < values.size(); ++i)
        {
                std::string method = values[i];
                // Convert to uppercase
                std::transform(method.begin(), method.end(), method.begin(), ::toupper);
                methods.insert(method);
        }
        return methods;
}

size_t Config::parseClientSizeDirective(const std::string &value)
{
        std::istringstream iss(value);
        size_t size;
        iss >> size;
        return size;
}

bool Config::parseAutoindexDirective(const std::string &value)
{
        return (value == "on" || value == "true" || value == "1");
}

std::vector<Config::ErrorPageConfig> Config::parseErrorPageDirective(const std::vector<std::string> &values)
{
        std::vector<ErrorPageConfig> errorPages;

        // Format: error_page 404 404.html 500 500.html;
        for (size_t i = 0; i < values.size(); i += 2)
        {
                if (i + 1 < values.size())
                {
                        ErrorPageConfig config;
                        std::istringstream iss(values[i]);
                        iss >> config.errorCode;
                        config.filePath = values[i + 1];
                        errorPages.push_back(config);
                }
        }

        return errorPages;
}
