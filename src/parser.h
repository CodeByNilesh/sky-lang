/* parser.h — Parser header */
#ifndef SKY_PARSER_H
#define SKY_PARSER_H

#include "lexer.h"
#include "ast.h"
#include <stdbool.h>

typedef struct {
    SkyLexer  *lexer;
    SkyToken   current;
    SkyToken   previous;
    bool       had_error;
    bool       panic_mode;
} SkyParser;

void        sky_parser_init(SkyParser *parser, SkyLexer *lexer);
SkyASTNode* sky_parser_parse(SkyParser *parser);
bool        sky_parser_had_error(const SkyParser *parser);

#endif
