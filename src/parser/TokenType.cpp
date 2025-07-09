#include "TokenType.hpp"

TokenType::TokenType() : value(ERROR) {}
TokenType::TokenType(e val) : value(val) {}
TokenType::operator e() const { return value; }
TokenType &TokenType::operator=(e val)
{
        value = val;
        return *this;
}

const char *tokenTypeToString(TokenType type)
{
        switch (type.value)
        {
        case TokenType::LEFT_BRACE:
                return "LEFT_BRACE";
        case TokenType::RIGHT_BRACE:
                return "RIGHT_BRACE";
        case TokenType::SEMICOLON:
                return "SEMICOLON";
        case TokenType::DIRECTIVE:
                return "DIRECTIVE";
        case TokenType::VALUE:
                return "VALUE";
        case TokenType::END_OF_FILE:
                return "END_OF_FILE";
        case TokenType::ERROR:
                return "ERROR";
        default:
                return "UNKNOWN";
        }
}
