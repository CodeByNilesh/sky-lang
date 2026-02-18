/* bytecode.h — Instruction set definitions */
#ifndef SKY_BYTECODE_H
#define SKY_BYTECODE_H

#include <stdint.h>
#include "value.h"

typedef enum {
    OP_NOP = 0,
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_TRUE,
    OP_FALSE,
    OP_NIL,
    OP_POP,
    OP_DUP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_FIELD,
    OP_SET_FIELD,
    OP_GET_INDEX,
    OP_SET_INDEX,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_NEGATE,
    OP_NOT,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_GREATER,
    OP_GREATER_EQ,
    OP_LESS,
    OP_LESS_EQ,
    OP_AND,
    OP_OR,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_JUMP_BACK,
    OP_CALL,
    OP_RETURN,
    OP_PRINT,
    OP_ARRAY,
    OP_MAP,
    OP_CLASS,
    OP_METHOD,
    OP_INVOKE,
    OP_IMPORT,
    OP_SERVER,
    OP_ROUTE,
    OP_RESPOND,
    OP_SECURITY,
    OP_ASYNC,
    OP_AWAIT,
    OP_HALT
} SkyOpCode;

typedef struct {
    uint8_t      *code;
    int           code_count;
    int           code_capacity;
    SkyValueArray constants;
    int          *lines;
} SkyChunk;

void sky_chunk_init(SkyChunk *chunk);
void sky_chunk_free(SkyChunk *chunk);
void sky_chunk_write(SkyChunk *chunk, uint8_t byte, int line);
int  sky_chunk_add_constant(SkyChunk *chunk, SkyValue value);

#endif
