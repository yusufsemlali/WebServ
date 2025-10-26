#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "Parser.hpp"
#include "Lexer.hpp"
#include "TokenType.hpp"

Config parse(const std::string &filePath)
{
        std::ifstream file(filePath.c_str());
        if (!file.is_open())
        {
                throw std::runtime_error("Could not open config file: " + filePath);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        Lexer lexer(content);
        std::vector<Token> tokens = lexer.tokenize();

        Parser parser(tokens);
        Config config = parser.parseConfig();

        return config;
}
