#pragma once

#include <string>
#include "TokenType.hpp"

struct Token {
        TokenType type;  // the type of the token
        std::string lexeme; // the actual text of the token
        int line;  // the line number where the token was found
};


