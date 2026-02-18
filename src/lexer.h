/* lexer.h — Lexer header */
#ifndef SKY_LEXER_H
#define SKY_LEXER_H

#include <stdbool.h>
#include "token.h"

typedef struct {
    const char *source;
    const char *start;
    const char *current;
    const char *filename;
    int         line;
    bool        had_error;
    char        error_msg[256];
} SkyLexer;

void     sky_lexer_init(SkyLexer *lexer, const char *source, const char *filename);
SkyToken sky_lexer_next(SkyLexer *lexer);
bool     sky_lexer_is_at_end(const SkyLexer *lexer);

#endif
