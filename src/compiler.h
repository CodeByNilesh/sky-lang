/* compiler.h — Bytecode compiler header */
#ifndef SKY_COMPILER_H
#define SKY_COMPILER_H

#include "bytecode.h"
#include "ast.h"
#include <stdbool.h>

#define SKY_MAX_LOCALS 256

typedef struct {
    char name[128];
    int  depth;
} Local;

typedef struct {
    SkyChunk   *chunk;
    Local       locals[SKY_MAX_LOCALS];
    int         local_count;
    int         scope_depth;
    bool        had_error;
} SkyCompiler;

void sky_compiler_init(SkyCompiler *compiler, SkyChunk *chunk);
bool sky_compiler_compile(SkyCompiler *compiler, SkyASTNode *ast);

#endif
