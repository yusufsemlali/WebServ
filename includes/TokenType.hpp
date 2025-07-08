#pragma once
// TokenType.h

struct TokenType
{
        // The enum itself is now nested inside the struct
        enum e
        {
                LEFT_BRACE,
                RIGHT_BRACE,
                SEMICOLON,
                DIRECTIVE,
                VALUE,
                END_OF_FILE,
                ERROR
        };

        e value;

        TokenType() : value(ERROR) {}
        TokenType(e val) : value(val) {}

        operator e() const { return value; }
        TokenType &operator=(e val)
        {
                value = val;
                return *this;
        }
};

inline const char *tokenTypeToString(TokenType type)
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
