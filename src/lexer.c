/* lexer.c — Tokenizer implementation */
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static bool is_at_end(SkyLexer *lex) {
    return *lex->current == '\0';
}

static char advance_char(SkyLexer *lex) {
    lex->current++;
    return lex->current[-1];
}

static char peek(SkyLexer *lex) {
    return *lex->current;
}

static char peek_next(SkyLexer *lex) {
    if (is_at_end(lex)) return '\0';
    return lex->current[1];
}

static bool match_char(SkyLexer *lex, char expected) {
    if (is_at_end(lex)) return false;
    if (*lex->current != expected) return false;
    lex->current++;
    return true;
}

static void skip_whitespace(SkyLexer *lex) {
    while (1) {
        char c = peek(lex);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance_char(lex);
                break;
            case '\n':
                lex->line++;
                advance_char(lex);
                break;
            case '/':
                if (peek_next(lex) == '/') {
                    while (peek(lex) != '\n' && !is_at_end(lex))
                        advance_char(lex);
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static SkyToken make_token(SkyLexer *lex, SkyTokenType type) {
    SkyToken token;
    int len;
    token.type = type;
    token.start = lex->start;
    token.length = (int)(lex->current - lex->start);
    token.line = lex->line;
    len = token.length;
    if (len >= 255) len = 255;
    memcpy(token.value, token.start, len);
    token.value[len] = '\0';
    return token;
}

static SkyToken error_token(SkyLexer *lex, const char *message) {
    SkyToken token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = lex->line;
    strncpy(token.value, message, 255);
    token.value[255] = '\0';
    lex->had_error = true;
    snprintf(lex->error_msg, sizeof(lex->error_msg), "%s", message);
    return token;
}

static SkyToken string_token(SkyLexer *lex) {
    int len;
    while (peek(lex) != '"' && !is_at_end(lex)) {
        if (peek(lex) == '\n') lex->line++;
        if (peek(lex) == '\\') advance_char(lex);
        advance_char(lex);
    }
    if (is_at_end(lex)) return error_token(lex, "Unterminated string");
    advance_char(lex);
    {
        SkyToken token;
        token.type = TOKEN_STRING_LITERAL;
        token.start = lex->start + 1;
        token.length = (int)(lex->current - lex->start - 2);
        token.line = lex->line;
        len = token.length;
        if (len >= 255) len = 255;
        memcpy(token.value, token.start, len);
        token.value[len] = '\0';
        return token;
    }
}

static SkyToken number_token(SkyLexer *lex) {
    bool is_float = false;
    while (isdigit((unsigned char)peek(lex))) advance_char(lex);
    if (peek(lex) == '.' && isdigit((unsigned char)peek_next(lex))) {
        is_float = true;
        advance_char(lex);
        while (isdigit((unsigned char)peek(lex))) advance_char(lex);
    }
    return make_token(lex, is_float ? TOKEN_FLOAT_LITERAL : TOKEN_INT_LITERAL);
}

static SkyTokenType check_keyword(SkyLexer *lex) {
    int length = (int)(lex->current - lex->start);
    struct { const char *name; SkyTokenType type; } keywords[] = {
        {"let",      TOKEN_LET},
        {"fn",       TOKEN_FN},
        {"return",   TOKEN_RETURN},
        {"if",       TOKEN_IF},
        {"else",     TOKEN_ELSE},
        {"for",      TOKEN_FOR},
        {"while",    TOKEN_WHILE},
        {"in",       TOKEN_IN},
        {"class",    TOKEN_CLASS},
        {"self",     TOKEN_SELF},
        {"true",     TOKEN_TRUE},
        {"false",    TOKEN_FALSE},
        {"nil",      TOKEN_NIL},
        {"print",    TOKEN_PRINT},
        {"import",   TOKEN_IMPORT},
        {"server",   TOKEN_SERVER},
        {"route",    TOKEN_ROUTE},
        {"respond",  TOKEN_RESPOND},
        {"on",       TOKEN_ON},
        {"security", TOKEN_SECURITY},
        {"async",    TOKEN_ASYNC},
        {"await",    TOKEN_AWAIT},
        {"break",    TOKEN_BREAK},
        {"continue", TOKEN_CONTINUE},
        {"not",      TOKEN_NOT},
        {"and",      TOKEN_AND},
        {"or",       TOKEN_OR},
        {NULL, TOKEN_EOF}
    };
    int i;
    for (i = 0; keywords[i].name != NULL; i++) {
        int kw_len = (int)strlen(keywords[i].name);
        if (kw_len == length && memcmp(lex->start, keywords[i].name, length) == 0) {
            return keywords[i].type;
        }
    }
    return TOKEN_IDENTIFIER;
}

static SkyToken identifier_token(SkyLexer *lex) {
    while (isalnum((unsigned char)peek(lex)) || peek(lex) == '_')
        advance_char(lex);
    return make_token(lex, check_keyword(lex));
}

void sky_lexer_init(SkyLexer *lexer, const char *source, const char *filename) {
    if (!lexer) return;
    /* Skip UTF-8 BOM if present */
    if ((unsigned char)source[0] == 0xEF &&
        (unsigned char)source[1] == 0xBB &&
        (unsigned char)source[2] == 0xBF) {
        source += 3;
    }
    lexer->source = source;
    lexer->start = source;
    lexer->current = source;
    lexer->filename = filename ? filename : "<stdin>";
    lexer->line = 1;
    lexer->had_error = false;
    memset(lexer->error_msg, 0, sizeof(lexer->error_msg));
}

SkyToken sky_lexer_next(SkyLexer *lex) {
    char c;
    skip_whitespace(lex);
    lex->start = lex->current;
    if (is_at_end(lex)) return make_token(lex, TOKEN_EOF);
    c = advance_char(lex);
    if (isdigit((unsigned char)c)) return number_token(lex);
    if (isalpha((unsigned char)c) || c == '_') return identifier_token(lex);
    switch (c) {
        case '(': return make_token(lex, TOKEN_LPAREN);
        case ')': return make_token(lex, TOKEN_RPAREN);
        case '{': return make_token(lex, TOKEN_LBRACE);
        case '}': return make_token(lex, TOKEN_RBRACE);
        case '[': return make_token(lex, TOKEN_LBRACKET);
        case ']': return make_token(lex, TOKEN_RBRACKET);
        case ',': return make_token(lex, TOKEN_COMMA);
        case ':': return make_token(lex, TOKEN_COLON);
        case ';': return make_token(lex, TOKEN_SEMICOLON);
        case '%': return make_token(lex, TOKEN_PERCENT);
        case '+': return make_token(lex, TOKEN_PLUS);
        case '-':
            if (match_char(lex, '>')) return make_token(lex, TOKEN_ARROW);
            return make_token(lex, TOKEN_MINUS);
        case '*': return make_token(lex, TOKEN_STAR);
        case '/': return make_token(lex, TOKEN_SLASH);
        case '.':
            if (match_char(lex, '.')) return make_token(lex, TOKEN_DOT_DOT);
            return make_token(lex, TOKEN_DOT);
        case '=':
            if (match_char(lex, '=')) return make_token(lex, TOKEN_EQUAL_EQUAL);
            return make_token(lex, TOKEN_ASSIGN);
        case '!':
            if (match_char(lex, '=')) return make_token(lex, TOKEN_NOT_EQUAL);
            return make_token(lex, TOKEN_BANG);
        case '<':
            if (match_char(lex, '=')) return make_token(lex, TOKEN_LESS_EQUAL);
            return make_token(lex, TOKEN_LESS);
        case '>':
            if (match_char(lex, '=')) return make_token(lex, TOKEN_GREATER_EQUAL);
            return make_token(lex, TOKEN_GREATER);
        case '&':
            if (match_char(lex, '&')) return make_token(lex, TOKEN_AND);
            return error_token(lex, "Expected '&&'");
        case '|':
            if (match_char(lex, '|')) return make_token(lex, TOKEN_OR);
            return error_token(lex, "Expected '||'");
        case '"': return string_token(lex);
    }
    return error_token(lex, "Unexpected character");
}

bool sky_lexer_is_at_end(const SkyLexer *lex) {
    return *lex->current == '\0';
}

