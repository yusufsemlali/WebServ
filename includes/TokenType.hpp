#pragma once
// TokenType.h

struct TokenType
{
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

        TokenType();
        TokenType(e val);

        operator e() const;
        TokenType &operator=(e val);
};

const char *tokenTypeToString(TokenType type);
