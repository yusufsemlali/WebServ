#pragma once

#include <string>
#include "TokenType.hpp"

struct Token {
        TokenType type;  
        std::string lexeme; 
        int line;  
};


