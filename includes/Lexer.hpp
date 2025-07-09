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
