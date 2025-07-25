#include "Lexer.hpp"
#include <iostream>

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
        directives.insert("methods");
        directives.insert("client_size");
        directives.insert("cgi_pass");
        // Add other directives here
        return directives;
}

const std::set<std::string> DIRECTIVES = initDirectives();

Lexer::Lexer(const std::string &source)
    : source(source), current(0), start(0), line(1) {}

Lexer::~Lexer() {}

std::vector<Token> Lexer::tokenize()
{
        std::vector<Token> tokens;
        while (!isAtEnd())
        {
                skipWhitespaceAndComments();
                if (isAtEnd())
                        break; 
                start = current;
                Token token = scanToken();
                if (token.type == TokenType::END_OF_FILE)
                        break; 
                tokens.push_back(token);
        }
        Token endToken;
        endToken.type = TokenType(TokenType::END_OF_FILE);
        endToken.lexeme = "";
        endToken.line = line;
        tokens.push_back(endToken);
        return tokens;
}

bool Lexer::isAtEnd() const
{
        return current >= source.length();
}

char Lexer::advance()
{
        return source[current++];
}

char Lexer::peek() const
{
        if (isAtEnd())
                return '\0';
        return source[current];
}

void Lexer::skipWhitespaceAndComments()
{
        while (true)
        {
                char c = peek();
                switch (c)
                {
                case ' ':
                case '\t':
                case '\r':
                        advance();
                        break;
                case '\n':
                        advance();
                        line++;
                        break;
                case '#':
                        while (peek() != '\n' && !isAtEnd())
                                advance();
                        break;
                case '/':
                        advance(); 
                        if (peek() == '/')
                        {
                                while (peek() != '\n' && !isAtEnd())
                                        advance();
                        }
                        else if (peek() == '*')
                        {
                                advance(); 
                                while (!isAtEnd())
                                {
                                        if (peek() == '*')
                                        {
                                                advance();
                                                if (peek() == '/')
                                                {
                                                        advance();
                                                        break;
                                                }
                                        }
                                        else
                                        {
                                                if (peek() == '\n')
                                                        line++;
                                                advance();
                                        }
                                }
                        }
                        else
                        {
                                current--; // Back up since this isn't a comment
                                return;
                        }
                        break;
                default:
                        return;
                }
        }
}

Token Lexer::makeToken(TokenType::e type)
{
        Token token;
        token.type = TokenType(type);
        token.lexeme = source.substr(start, current - start);
        token.line = line;
        return token;
}

Token Lexer::scanToken()
{
        // Remember to set 'start = current;' before calling this

        if (isAtEnd())
        {
                // If we're at the end, return an empty token that won't be processed
                Token token;
                token.type = TokenType(TokenType::END_OF_FILE);
                token.lexeme = "";
                token.line = line;
                return token;
        }

        char c = advance();
        switch (c)
        {
        case '{':
                return makeToken(TokenType::LEFT_BRACE);
        case '}':
                return makeToken(TokenType::RIGHT_BRACE);
        case ';':
                return makeToken(TokenType::SEMICOLON);

        default:
                current--;
                return makeWord();
        }
}

Token Lexer::makeWord()
{
        while (!isAtEnd() && !isspace(peek()) && peek() != ';' && peek() != '{' && peek() != '}')
        {
                advance();
        }

        std::string lexeme = source.substr(start, current - start);

        // After getting the whole word, decide if it's a directive or a value
        if (DIRECTIVES.count(lexeme))
        {
                Token token;
                token.type = TokenType(TokenType::DIRECTIVE);
                token.lexeme = lexeme;
                token.line = line;
                return token;
        }

        Token token;
        token.type = TokenType(TokenType::VALUE);
        token.lexeme = lexeme;
        token.line = line;
        return token;
}