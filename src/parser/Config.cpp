#include "Config.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

Config::Config() {}

Config::~Config() {}

void Config::validateAndParseConfig()
{
        if (servers.empty())
        {
                throw std::runtime_error("Configuration must contain at least one server block");
        }

        for (size_t i = 0; i < servers.size(); ++i)
        {
                parseServerConfig(servers[i]);
        }
}

void Config::parseServerConfig(ServerConfig &server)
{
        for (std::map<std::string, std::vector<std::string> >::iterator it = server.directives.begin();
             it != server.directives.end(); ++it)
        {
                validateDirectiveValues(it->first, it->second);
        }

        if (server.directives.find("listen") != server.directives.end())
        {
                const std::vector<std::string> &listenValues = server.directives["listen"];
                for (size_t i = 0; i < listenValues.size(); ++i)
                {
                        server.listenConfigs.push_back(parseListenDirective(listenValues[i]));
                }
        }
        else
        {
                ListenConfig defaultConfig;
                defaultConfig.host = "0.0.0.0";
                defaultConfig.port = "8080";
                defaultConfig.isDefault = true;
                server.listenConfigs.push_back(defaultConfig);
        }

        validateDuplicateListenAddresses(server.listenConfigs);

        if (server.directives.find("server_name") != server.directives.end())
        {
                server.serverNames = server.directives["server_name"];
        }

        if (server.directives.find("root") != server.directives.end())
        {
                server.root = server.directives["root"].back();
        }

        if (server.directives.find("index") != server.directives.end())
        {
                server.index = server.directives["index"].back();
        }

        if (server.directives.find("methods") != server.directives.end())
        {
                server.allowedMethods = parseMethodsDirective(server.directives["methods"]);
        }

        if (server.directives.find("autoindex") != server.directives.end())
        {
                server.autoindex = parseAutoindexDirective(server.directives["autoindex"].back());
        }
        else
        {
                server.autoindex = false;
        }

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
                server.clientMaxBodySize = 1048576;
        }

        if (server.directives.find("error_page") != server.directives.end())
        {
                server.errorPages = parseErrorPageDirective(server.directives["error_page"]);
        }

        for (size_t i = 0; i < server.locations.size(); ++i)
        {
                parseLocationConfig(server.locations[i]);
        }

        validateLocationRedirects(server);
}

void Config::parseLocationConfig(LocationConfig &location)
{
        for (std::map<std::string, std::vector<std::string> >::iterator it = location.directives.begin();
             it != location.directives.end(); ++it)
        {
                validateDirectiveValues(it->first, it->second);
        }

        if (location.directives.find("methods") != location.directives.end())
        {
                location.allowedMethods = parseMethodsDirective(location.directives["methods"]);
        }

        if (location.directives.find("root") != location.directives.end())
        {
                location.root = location.directives["root"].back();
        }

        if (location.directives.find("index") != location.directives.end())
        {
                location.index = location.directives["index"].back();
        }

        if (location.directives.find("autoindex") != location.directives.end())
        {
                location.autoindex = parseAutoindexDirective(location.directives["autoindex"].back());
        }
        else
        {
                location.autoindex = false;
        }

        if (location.directives.find("client_size") != location.directives.end())
        {
                location.clientMaxBodySize = parseClientSizeDirective(location.directives["client_size"].back());
        }
        else
        {
                location.clientMaxBodySize = 0;
        }

        if (location.directives.find("cgi_pass") != location.directives.end())
        {
                location.cgiPass = location.directives["cgi_pass"].back();
        }

        if (location.directives.find("return") != location.directives.end())
        {
                location.returnUrl = location.directives["return"].back();
        }

        if (location.directives.find("error_page") != location.directives.end())
        {
                location.errorPages = parseErrorPageDirective(location.directives["error_page"]);
        }
}

Config::ListenConfig Config::parseListenDirective(const std::string &value)
{
        ListenConfig config;
        config.host = "0.0.0.0";
        config.port = "8080";
        config.isDefault = false;

        if (value.find(':') != std::string::npos)
        {
                size_t colonPos = value.find(':');
                std::string host = value.substr(0, colonPos);
                config.host = (host == "*") ? "0.0.0.0" : host;
                config.port = value.substr(colonPos + 1);
        }
        else
        {
                bool isNumeric = true;
                for (size_t i = 0; i < value.length(); ++i)
                {
                        if (!std::isdigit(value[i]))
                        {
                                isNumeric = false;
                                break;
                        }
                }

                if (isNumeric)
                {
                        config.port = value;
                }
                else
                {
                        config.host = (value == "*") ? "0.0.0.0" : value;
                        config.port = "8080";
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
                std::transform(method.begin(), method.end(), method.begin(), ::toupper);
                methods.insert(method);
        }
        return methods;
}

size_t Config::parseClientSizeDirective(const std::string &value)
{
        if (value.empty())
        {
                throwValidationError("client_size", value, "size value cannot be empty");
        }
        
        std::string val = value;
        size_t multiplier = 1;
        
        // Check for suffix (k, m, g - case insensitive)
        char lastChar = val[val.length() - 1];
        if (!isdigit(lastChar))
        {
                char suffix = tolower(lastChar);
                if (suffix == 'k')
                {
                        multiplier = 1024;  // KB
                }
                else if (suffix == 'm')
                {
                        multiplier = 1024 * 1024;  // MB
                }
                else if (suffix == 'g')
                {
                        multiplier = 1024 * 1024 * 1024;  // GB
                }
                else if (suffix == 'b' && val.length() > 1)
                {
                        // Handle "mb", "kb", "gb" suffixes
                        char secondLast = tolower(val[val.length() - 2]);
                        if (secondLast == 'k')
                        {
                                multiplier = 1024;
                                val = val.substr(0, val.length() - 2);
                        }
                        else if (secondLast == 'm')
                        {
                                multiplier = 1024 * 1024;
                                val = val.substr(0, val.length() - 2);
                        }
                        else if (secondLast == 'g')
                        {
                                multiplier = 1024 * 1024 * 1024;
                                val = val.substr(0, val.length() - 2);
                        }
                        else
                        {
                                throwValidationError("client_size", value, "invalid size suffix (use k, m, g, kb, mb, or gb)");
                        }
                }
                else
                {
                        throwValidationError("client_size", value, "invalid size suffix (use k, m, g, kb, mb, or gb)");
                }
                
                // Remove suffix if it was single character
                if (multiplier > 1 && val.length() > 1 && !isdigit(val[val.length() - 1]))
                {
                        val = val.substr(0, val.length() - 1);
                }
        }
        
        // Parse the numeric part
        std::istringstream iss(val);
        size_t size;
        if (!(iss >> size))
        {
                throwValidationError("client_size", value, "size must be a valid number");
        }
        
        // Check for overflow
        if (size > (size_t)-1 / multiplier)
        {
                throwValidationError("client_size", value, "size value too large (overflow)");
        }
        
        return size * multiplier;
}

bool Config::parseAutoindexDirective(const std::string &value)
{
        return (value == "on" || value == "true" || value == "1");
}

std::vector<Config::ErrorPageConfig> Config::parseErrorPageDirective(const std::vector<std::string> &values)
{
        std::vector<ErrorPageConfig> errorPages;

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

void Config::validateDirectiveValues(const std::string &directive, const std::vector<std::string> &values)
{
        if (values.empty())
        {
                throwValidationError(directive, "", "directive cannot be empty");
        }

        if (directive == "listen")
        {
                for (size_t i = 0; i < values.size(); ++i)
                {
                        validateListenValue(values[i]);
                }
        }
        else if (directive == "methods")
        {
                validateHttpMethods(values);
        }
        else if (directive == "server_name")
        {
                for (size_t i = 0; i < values.size(); ++i)
                {
                        validateServerName(values[i]);
                }
        }
        else if (directive == "root")
        {
                if (values.size() != 1)
                {
                        throwValidationError(directive, "", "root directive must have exactly one value");
                }
                validateFilePath(values[0]);
        }
        else if (directive == "index")
        {
                if (values.size() != 1)
                {
                        throwValidationError(directive, "", "index directive must have exactly one value");
                }
                if (values[0].empty() || values[0].find('/') != std::string::npos)
                {
                        throwValidationError(directive, values[0], "index must be a filename without path separators");
                }
        }
        else if (directive == "client_size" || directive == "client_max_body_size")
        {
                if (values.size() != 1)
                {
                        throwValidationError(directive, "", directive + " directive must have exactly one value");
                }
                validateClientSize(values[0]);
        }
        else if (directive == "autoindex")
        {
                if (values.size() != 1)
                {
                        throwValidationError(directive, "", "autoindex directive must have exactly one value");
                }
                validateAutoindexValue(values[0]);
        }
        else if (directive == "return")
        {
                if (values.size() != 1)
                {
                        throwValidationError(directive, "", "return directive must have exactly one value");
                }
                validateReturnValue(values[0]);
        }
        else if (directive == "cgi_pass")
        {
                if (values.size() != 1)
                {
                        throwValidationError(directive, "", "cgi_pass directive must have exactly one value");
                }
                validateCgiPath(values[0]);
        }
        else if (directive == "error_page")
        {
                if (values.size() % 2 != 0)
                {
                        throwValidationError(directive, "", "error_page directive must have pairs of error_code and file_path");
                }
                for (size_t i = 0; i < values.size(); i += 2)
                {
                        std::istringstream iss(values[i]);
                        int errorCode;
                        if (!(iss >> errorCode))
                        {
                                throwValidationError(directive, values[i], "error code must be a number");
                        }
                        validateErrorCode(errorCode);
                        validateFilePath(values[i + 1]);
                }
        }
}

void Config::validateHttpMethods(const std::vector<std::string> &methods)
{
        if (methods.empty())
        {
                throwValidationError("methods", "", "methods directive cannot be empty");
        }

        for (size_t i = 0; i < methods.size(); ++i)
        {
                if (!isValidHttpMethod(methods[i]))
                {
                        throwValidationError("methods", methods[i], "invalid HTTP method. Valid methods: GET, POST, DELETE, PUT, HEAD, OPTIONS");
                }
        }
}

void Config::validateListenValue(const std::string &value)
{
        if (value.find(':') != std::string::npos)
        {
                size_t colonPos = value.find(':');
                std::string host = value.substr(0, colonPos);
                std::string portStr = value.substr(colonPos + 1);

                validateHostname(host);

                std::istringstream iss(portStr);
                int port;
                if (!(iss >> port))
                {
                        throwValidationError("listen", value, "port must be a number");
                }
                validatePortNumber(port);
        }
        else
        {
                std::istringstream iss(value);
                int port;
                if (!(iss >> port))
                {
                        validateHostname(value);
                }
                else
                {
                        validatePortNumber(port);
                }
        }
}

void Config::validatePortNumber(int port)
{
        if (!isValidPort(port))
        {
                std::ostringstream oss;
                oss << port;
                throwValidationError("port", oss.str(), "port must be between 1 and 65535");
        }
}

void Config::validateHostname(const std::string &host)
{
        if (!isValidHostname(host))
        {
                throwValidationError("hostname", host, "invalid hostname format");
        }
}

void Config::validateFilePath(const std::string &path)
{
        if (!isValidFilePath(path))
        {
                throwValidationError("file_path", path, "invalid file path format");
        }
}

void Config::validateServerName(const std::string &serverName)
{
        if (serverName.empty())
        {
                throwValidationError("server_name", serverName, "server name cannot be empty");
        }
        if (serverName.find(' ') != std::string::npos)
        {
                throwValidationError("server_name", serverName, "server name cannot contain spaces");
        }
}

void Config::validateErrorCode(int errorCode)
{
        if (!isValidErrorCode(errorCode))
        {
                std::ostringstream oss;
                oss << errorCode;
                throwValidationError("error_code", oss.str(), "error code must be a valid HTTP status code (400-599)");
        }
}

void Config::validateClientSize(const std::string &value)
{
        std::istringstream iss(value);
        size_t size;
        if (!(iss >> size))
        {
                throwValidationError("client_size", value, "client size must be a number");
        }
        if (size > 1073741824)
        {
                throwValidationError("client_size", value, "client size cannot exceed 1GB (1073741824 bytes)");
        }
}

void Config::validateAutoindexValue(const std::string &value)
{
        if (value != "on" && value != "off" && value != "true" && value != "false" && value != "1" && value != "0")
        {
                throwValidationError("autoindex", value, "autoindex must be 'on', 'off', 'true', 'false', '1', or '0'");
        }
}

void Config::validateReturnValue(const std::string &value)
{
        if (!isValidReturnValue(value))
        {
                throwValidationError("return", value, "return value must be a valid URL or path starting with '/'");
        }
}

void Config::validateCgiPath(const std::string &path)
{
        if (path.empty())
        {
                throwValidationError("cgi_pass", path, "CGI path cannot be empty");
        }
        if (path[0] != '.' && path[1] != '/')
        {
                throwValidationError("cgi_pass", path, "CGI path must be a relative path starting with './'" );
        }
}

void Config::validateDuplicateListenAddresses(const std::vector<ListenConfig> &listenConfigs)
{
        std::set<std::string> seenAddresses;
        std::map<std::string, std::vector<std::string> > portToHosts;
        
        for (size_t i = 0; i < listenConfigs.size(); ++i)
        {
                const std::string &host = listenConfigs[i].host;
                const std::string &port = listenConfigs[i].port;
                std::string addressKey = host + ":" + port;
                
                if (seenAddresses.find(addressKey) != seenAddresses.end())
                {
                        throwValidationError("listen", addressKey, 
                                "duplicate listen directive with same IP:port combination");
                }
                
                seenAddresses.insert(addressKey);
                portToHosts[port].push_back(host);
        }
        
        for (std::map<std::string, std::vector<std::string> >::iterator it = portToHosts.begin();
             it != portToHosts.end(); ++it)
        {
                const std::string &port = it->first;
                const std::vector<std::string> &hosts = it->second;
                
                if (hosts.size() > 1)
                {
                        bool hasWildcard = false;
                        bool hasSpecific = false;
                        
                        for (size_t i = 0; i < hosts.size(); ++i)
                        {
                                if (hosts[i] == "0.0.0.0")
                                {
                                        hasWildcard = true;
                                }
                                else
                                {
                                        hasSpecific = true;
                                }
                        }
                        
                        if (hasWildcard && hasSpecific)
                        {
                                throwValidationError("listen", "port " + port,
                                        "wildcard address 0.0.0.0 conflicts with specific IP address on same port");
                        }
                }
        }
}

bool Config::isValidHttpMethod(const std::string &method)
{
        return (method == "GET" || method == "POST" || method == "DELETE" ||
                method == "PUT" || method == "HEAD" || method == "OPTIONS");
}

bool Config::isValidErrorCode(int code)
{
        return (code >= 400 && code <= 599);
}

bool Config::isValidPort(int port)
{
        return (port >= 1 && port <= 65535);
}

bool Config::isValidHostname(const std::string &host)
{
        if (host.empty() || host.length() > 253)
                return false;

        if (host == "*" || host == "localhost" || host == "0.0.0.0")
                return true;

        if (host.find_first_not_of("0123456789.") == std::string::npos)
        {
                size_t dotCount = 0;
                for (size_t i = 0; i < host.length(); ++i)
                {
                        if (host[i] == '.')
                                dotCount++;
                }
                return dotCount == 3;
        }

        return host.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-") == std::string::npos;
}

bool Config::isValidFilePath(const std::string &path)
{
        if (path.empty())
                return false;

        if (path[0] != '.' || path[1] != '/')
                return false;

        if (path.find("..") != std::string::npos)
                return false;

        return true;
}

bool Config::isValidUrl(const std::string &url)
{
        return (url.find("http://") == 0 || url.find("https://") == 0);
}

bool Config::isValidReturnValue(const std::string &value)
{
        return isValidUrl(value) || (value.length() > 0 && value[0] == '/');
}

void Config::validateLocationRedirects(const ServerConfig &server)
{
        const std::vector<LocationConfig> &locations = server.locations;
        
        for (size_t i = 0; i < locations.size(); ++i)
        {
                const std::string &returnUrl = locations[i].returnUrl;
                
                if (returnUrl.empty() || isValidUrl(returnUrl))
                        continue;
                
                std::set<std::string> visitedPaths;
                std::string currentPath = locations[i].path;
                std::string nextPath = returnUrl;
                
                visitedPaths.insert(currentPath);
                
                while (!nextPath.empty())
                {
                        if (visitedPaths.find(nextPath) != visitedPaths.end())
                        {
                                throwValidationError("return", locations[i].path,
                                        "circular redirect detected: " + locations[i].path + " -> ... -> " + nextPath);
                        }
                        
                        visitedPaths.insert(nextPath);
                        
                        const LocationConfig *nextLocation = NULL;
                        for (size_t j = 0; j < locations.size(); ++j)
                        {
                                if (locations[j].path == nextPath)
                                {
                                        nextLocation = &locations[j];
                                        break;
                                }
                        }
                        
                        if (!nextLocation || nextLocation->returnUrl.empty() || isValidUrl(nextLocation->returnUrl))
                        {
                                break;
                        }
                        
                        nextPath = nextLocation->returnUrl;
                }
        }
}

void Config::throwValidationError(const std::string &directive, const std::string &value, const std::string &reason)
{
        std::string error = "Configuration validation error in directive '" + directive + "'";
        if (!value.empty())
        {
                error += " with value '" + value + "'";
        }
        error += ": " + reason;
        throw std::runtime_error(error);
}
