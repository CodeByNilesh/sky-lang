/* token.h — Token type definitions */
#ifndef SKY_TOKEN_H
#define SKY_TOKEN_H

typedef enum {
    /* Literals */
    TOKEN_INT_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_IDENTIFIER,

    /* Operators */
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_ASSIGN,
    TOKEN_EQUAL_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_DOT,
    TOKEN_DOT_DOT,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_ARROW,
    TOKEN_BANG,

    /* Delimiters */
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,

    /* Keywords */
    TOKEN_LET,
    TOKEN_FN,
    TOKEN_RETURN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_IN,
    TOKEN_CLASS,
    TOKEN_SELF,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NIL,
    TOKEN_PRINT,
    TOKEN_IMPORT,
    TOKEN_SERVER,
    TOKEN_ROUTE,
    TOKEN_RESPOND,
    TOKEN_ON,
    TOKEN_SECURITY,
    TOKEN_ASYNC,
    TOKEN_AWAIT,
    TOKEN_BREAK,
    TOKEN_CONTINUE,

    /* Special */
    TOKEN_ERROR,
    TOKEN_EOF
} SkyTokenType;

typedef struct {
    SkyTokenType type;
    const char  *start;
    int          length;
    int          line;
    char         value[256];
} SkyToken;

#endif
