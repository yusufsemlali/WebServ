#pragma once

#include <string>
#include <vector>

#include "Config.hpp"
#include "Lexer.hpp"

// Entry point function for parsing config files
Config parse(const std::string &filePath);

class Parser
{
       public:
	Parser(const std::vector<Token> &tokens);
	~Parser();

	Config parseConfig();

       private:
	const std::vector<Token> &tokens;
	size_t current;

	// Helper methods
	bool isAtEnd() const;
	Token peek() const;
	Token previous() const;
	Token advance();
	bool check(TokenType::e type) const;
	bool match(TokenType::e type);
	void consume(TokenType::e type, const std::string &message);

	// Parsing methods
	Config::ServerConfig parseServer();
	Config::LocationConfig parseLocation();
	void parseDirective(std::map<std::string, std::vector<std::string> > &directives);

	// Error handling
	void error(const std::string &message) const;
};
