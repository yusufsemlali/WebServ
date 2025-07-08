#pragma once

#include "TokenType.hpp"
#include "Token.hpp"
#include <string>
#include <vector>
#include <set>

class Lexer
{
public:
        Lexer(const std::string &source);
        std::vector<Token> tokenize();
        ~Lexer();

private:
        const std::string &source;          // the source code to tokenize
        size_t current;                     // the current position in the source code
        size_t start;                       // the start of the current token
        int line;                           // the current line number
        bool isAtEnd() const;               // check if the end of the source code has been reached
        char advance();                     // advance to the next character
        char peek() const;                  // look at the next character without consuming it
        void skipWhitespaceAndComments();   // skip whitespace and comments
        Token scanToken();                  // scan the next token from the source code
        Token makeWord();                   // create a token from a word (directive or value)
        Token makeToken(TokenType::e type); // create a token of a specific type
};

static std::set<std::string> initDirectives()
{
        std::set<std::string> directives;
        directives.insert("listen");
        directives.insert("server");
        directives.insert("location");
        directives.insert("root");
        directives.insert("index");
        directives.insert("error_page");
        directives.insert("return");
        directives.insert("server_name");
        directives.insert("client_max_body_size");
        directives.insert("access_log");
        directives.insert("error_log");
        directives.insert("proxy_pass");
        directives.insert("try_files");
        directives.insert("rewrite");
        directives.insert("ssl_certificate");
        directives.insert("ssl_certificate_key");
        directives.insert("autoindex");
        directives.insert("alias");
        // Add other directives here
        return directives;
}

static const std::set<std::string> DIRECTIVES = initDirectives();