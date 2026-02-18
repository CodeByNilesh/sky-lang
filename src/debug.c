/* debug.c — Debug utilities implementation */
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool sky_debug_trace_execution = false;

static const char* op_name(uint8_t op) {
    switch (op) {
        case OP_NOP: return "NOP";
        case OP_CONSTANT: return "CONSTANT";
        case OP_CONSTANT_LONG: return "CONSTANT_LONG";
        case OP_TRUE: return "TRUE";
        case OP_FALSE: return "FALSE";
        case OP_NIL: return "NIL";
        case OP_POP: return "POP";
        case OP_DUP: return "DUP";
        case OP_GET_LOCAL: return "GET_LOCAL";
        case OP_SET_LOCAL: return "SET_LOCAL";
        case OP_GET_GLOBAL: return "GET_GLOBAL";
        case OP_SET_GLOBAL: return "SET_GLOBAL";
        case OP_GET_FIELD: return "GET_FIELD";
        case OP_SET_FIELD: return "SET_FIELD";
        case OP_GET_INDEX: return "GET_INDEX";
        case OP_SET_INDEX: return "SET_INDEX";
        case OP_ADD: return "ADD";
        case OP_SUB: return "SUB";
        case OP_MUL: return "MUL";
        case OP_DIV: return "DIV";
        case OP_MOD: return "MOD";
        case OP_NEGATE: return "NEGATE";
        case OP_NOT: return "NOT";
        case OP_EQUAL: return "EQUAL";
        case OP_NOT_EQUAL: return "NOT_EQUAL";
        case OP_GREATER: return "GREATER";
        case OP_GREATER_EQ: return "GREATER_EQ";
        case OP_LESS: return "LESS";
        case OP_LESS_EQ: return "LESS_EQ";
        case OP_AND: return "AND";
        case OP_OR: return "OR";
        case OP_JUMP: return "JUMP";
        case OP_JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case OP_JUMP_BACK: return "JUMP_BACK";
        case OP_CALL: return "CALL";
        case OP_RETURN: return "RETURN";
        case OP_PRINT: return "PRINT";
        case OP_ARRAY: return "ARRAY";
        case OP_MAP: return "MAP";
        case OP_CLASS: return "CLASS";
        case OP_METHOD: return "METHOD";
        case OP_INVOKE: return "INVOKE";
        case OP_IMPORT: return "IMPORT";
        case OP_SERVER: return "SERVER";
        case OP_ROUTE: return "ROUTE";
        case OP_RESPOND: return "RESPOND";
        case OP_SECURITY: return "SECURITY";
        case OP_ASYNC: return "ASYNC";
        case OP_AWAIT: return "AWAIT";
        case OP_HALT: return "HALT";
        default: return "UNKNOWN";
    }
}

int sky_disassemble_instruction(SkyChunk *chunk, int offset) {
    uint8_t op;
    printf("%04d ", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }
    op = chunk->code[offset];
    printf("%-20s", op_name(op));
    switch (op) {
        case OP_CONSTANT:
        case OP_GET_LOCAL:
        case OP_SET_LOCAL:
        case OP_GET_GLOBAL:
        case OP_SET_GLOBAL:
        case OP_GET_FIELD:
        case OP_SET_FIELD:
        case OP_CALL:
        case OP_ARRAY:
        case OP_MAP:
        case OP_INVOKE: {
            uint8_t idx = chunk->code[offset + 1];
            printf(" %4d", idx);
            if (op == OP_CONSTANT && idx < chunk->constants.count) {
                printf("  (");
                sky_debug_print_value(chunk->constants.values[idx]);
                printf(")");
            }
            printf("\n");
            return offset + 2;
        }
        case OP_CONSTANT_LONG: {
            uint32_t idx = ((uint32_t)chunk->code[offset + 1] << 16) |
                           ((uint32_t)chunk->code[offset + 2] << 8) |
                           (uint32_t)chunk->code[offset + 3];
            printf(" %4d", idx);
            if ((int)idx < chunk->constants.count) {
                printf("  (");
                sky_debug_print_value(chunk->constants.values[idx]);
                printf(")");
            }
            printf("\n");
            return offset + 4;
        }
        case OP_JUMP:
        case OP_JUMP_IF_FALSE: {
            uint16_t target = ((uint16_t)chunk->code[offset + 1] << 8) |
                              (uint16_t)chunk->code[offset + 2];
            printf(" -> %04d\n", offset + 3 + target);
            return offset + 3;
        }
        case OP_JUMP_BACK: {
            uint16_t target = ((uint16_t)chunk->code[offset + 1] << 8) |
                              (uint16_t)chunk->code[offset + 2];
            printf(" -> %04d\n", offset + 3 - target);
            return offset + 3;
        }
        default:
            printf("\n");
            return offset + 1;
    }
}

void sky_disassemble_chunk(SkyChunk *chunk, const char *name) {
    int offset = 0;
    printf("=== %s ===\n", name);
    while (offset < chunk->code_count) {
        offset = sky_disassemble_instruction(chunk, offset);
    }
    printf("=== end %s ===\n\n", name);
}

void sky_debug_print_value(SkyValue value) {
    switch (value.type) {
        case VAL_INT: printf("%lld", (long long)value.as.integer); break;
        case VAL_FLOAT: printf("%g", value.as.floating); break;
        case VAL_BOOL: printf("%s", value.as.boolean ? "true" : "false"); break;
        case VAL_NIL: printf("nil"); break;
        case VAL_STRING: printf("\"%s\"", value.as.string ? value.as.string : ""); break;
        case VAL_ARRAY: printf("[array]"); break;
        case VAL_MAP: printf("{map}"); break;
        case VAL_FUNCTION: printf("<fn>"); break;
        case VAL_NATIVE_FN: printf("<native>"); break;
        case VAL_CLASS: printf("<class>"); break;
        case VAL_INSTANCE: printf("<instance>"); break;
        default: printf("<unknown>"); break;
    }
}

void sky_debug_print_stack(SkyValue *stack, int stack_top) {
    int i;
    printf("          ");
    for (i = 0; i < stack_top; i++) {
        printf("[ ");
        sky_debug_print_value(stack[i]);
        printf(" ]");
    }
    printf("\n");
}
