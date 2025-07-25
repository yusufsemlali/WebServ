#pragma once

#include <string>
#include <vector>

#include "Token.hpp"
#include "TokenType.hpp"

class Lexer
{
       public:
	Lexer(const std::string &source);
	std::vector<Token> tokenize();
	~Lexer();

       private:
	const std::string &source;
	size_t current;
	size_t start;
	int line;
	bool isAtEnd() const;
	char advance();
	char peek() const;
	void skipWhitespaceAndComments();
	Token scanToken();
	Token makeWord();
	Token makeToken(TokenType::e type);
};
