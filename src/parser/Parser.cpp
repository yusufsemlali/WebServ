#include "Parser.hpp"
#include <iostream>
#include <sstream>

Parser::Parser(const std::vector<Token> &tokens)
    : tokens(tokens), current(0) {}

Parser::~Parser() {}

Config Parser::parseConfig()
{
        Config config;

        while (!isAtEnd())
        {
                if (check(TokenType::DIRECTIVE) && peek().lexeme == "server")
                {
                        config.servers.push_back(parseServer());
                }
                else
                {
                        error("Expected 'server' directive at top level");
                }
        }

        return config;
}

bool Parser::isAtEnd() const
{
        return peek().type == TokenType::END_OF_FILE;
}

Token Parser::peek() const
{
        return tokens[current];
}

Token Parser::previous() const
{
        return tokens[current - 1];
}

Token Parser::advance()
{
        if (!isAtEnd())
                current++;
        return previous();
}

bool Parser::check(TokenType::e type) const
{
        if (isAtEnd())
                return false;
        return peek().type == type;
}

bool Parser::match(TokenType::e type)
{
        if (check(type))
        {
                advance();
                return true;
        }
        return false;
}

void Parser::consume(TokenType::e type, const std::string &message)
{
        if (check(type))
        {
                advance();
                return;
        }
        error(message);
}

Config::ServerConfig Parser::parseServer()
{
        Config::ServerConfig server;

        // Consume 'server' directive
        consume(TokenType::DIRECTIVE, "Expected 'server' directive");
        consume(TokenType::LEFT_BRACE, "Expected '{' after 'server'");

        while (!check(TokenType::RIGHT_BRACE) && !isAtEnd())
        {
                if (check(TokenType::DIRECTIVE))
                {
                        std::string directive = peek().lexeme;

                        if (directive == "location")
                        {
                                server.locations.push_back(parseLocation());
                        }
                        else
                        {
                                parseDirective(server.directives);
                        }
                }
                else
                {
                        error("Expected directive inside server block");
                }
        }

        consume(TokenType::RIGHT_BRACE, "Expected '}' after server block");

        return server;
}

Config::LocationConfig Parser::parseLocation()
{
        Config::LocationConfig location;

        // Consume 'location' directive
        consume(TokenType::DIRECTIVE, "Expected 'location' directive");

        // Get the path
        if (check(TokenType::VALUE))
        {
                location.path = advance().lexeme;
        }
        else
        {
                error("Expected path after 'location' directive");
        }

        consume(TokenType::LEFT_BRACE, "Expected '{' after location path");

        while (!check(TokenType::RIGHT_BRACE) && !isAtEnd())
        {
                if (check(TokenType::DIRECTIVE))
                {
                        parseDirective(location.directives);
                }
                else
                {
                        error("Expected directive inside location block");
                }
        }

        consume(TokenType::RIGHT_BRACE, "Expected '}' after location block");

        return location;
}

void Parser::parseDirective(std::map<std::string, std::vector<std::string> > &directives)
{
        if (!check(TokenType::DIRECTIVE))
        {
                error("Expected directive");
        }

        std::string directiveName = advance().lexeme;

        if (!check(TokenType::VALUE))
        {
                error("Expected value after directive '" + directiveName + "'");
        }

        std::string value = advance().lexeme;

        consume(TokenType::SEMICOLON, "Expected ';' after directive value");

        // Add the value to the directive (supports multiple values)
        directives[directiveName].push_back(value);
}

void Parser::error(const std::string &message) const
{
        Token current_token = peek();

        // Convert line number to string (C++98 compatible)
        std::ostringstream oss;
        oss << current_token.line;

        std::string error_msg = "Parser error at line " +
                                oss.str() +
                                ": " + message;

        if (!isAtEnd())
        {
                error_msg += " (found: '" + current_token.lexeme + "')";
        }

        throw std::runtime_error(error_msg);
}
