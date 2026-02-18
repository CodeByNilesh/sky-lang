/* compiler.c — Bytecode compiler implementation */
#include "compiler.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void emit_byte(SkyCompiler *c, uint8_t byte, int line) {
    sky_chunk_write(c->chunk, byte, line);
}

static void emit_bytes(SkyCompiler *c, uint8_t b1, uint8_t b2, int line) {
    emit_byte(c, b1, line);
    emit_byte(c, b2, line);
}

static int emit_jump(SkyCompiler *c, uint8_t op, int line) {
    emit_byte(c, op, line);
    emit_byte(c, 0xff, line);
    emit_byte(c, 0xff, line);
    return c->chunk->code_count - 2;
}

static void patch_jump(SkyCompiler *c, int offset) {
    int jump = c->chunk->code_count - offset - 2;
    if (jump > 0xffff) {
        fprintf(stderr, "Compiler error: Jump too large\n");
        c->had_error = true;
        return;
    }
    c->chunk->code[offset]     = (jump >> 8) & 0xff;
    c->chunk->code[offset + 1] = jump & 0xff;
}

static int make_constant(SkyCompiler *c, SkyValue val) {
    int idx = sky_chunk_add_constant(c->chunk, val);
    if (idx > 255) {
        fprintf(stderr, "Compiler error: Too many constants\n");
        c->had_error = true;
        return 0;
    }
    return idx;
}

static void emit_constant(SkyCompiler *c, SkyValue val, int line) {
    emit_bytes(c, OP_CONSTANT, (uint8_t)make_constant(c, val), line);
}

static int add_local(SkyCompiler *c, const char *name) {
    if (c->local_count >= SKY_MAX_LOCALS) {
        fprintf(stderr, "Compiler error: Too many local variables\n");
        c->had_error = true;
        return -1;
    }
    Local *local = &c->locals[c->local_count];
    strncpy(local->name, name, sizeof(local->name) - 1);
    local->name[sizeof(local->name) - 1] = '\0';
    local->depth = c->scope_depth;
    return c->local_count++;
}

static int resolve_local(SkyCompiler *c, const char *name) {
    int i;
    for (i = c->local_count - 1; i >= 0; i--) {
        if (strcmp(c->locals[i].name, name) == 0) return i;
    }
    return -1;
}

static void begin_scope(SkyCompiler *c) {
    c->scope_depth++;
}

static void end_scope(SkyCompiler *c, int line) {
    c->scope_depth--;
    while (c->local_count > 0 && c->locals[c->local_count - 1].depth > c->scope_depth) {
        emit_byte(c, OP_POP, line);
        c->local_count--;
    }
}

static void compile_node(SkyCompiler *c, SkyASTNode *node);

static void compile_block(SkyCompiler *c, SkyASTNode *node) {
    int i;
    if (!node) return;
    if (node->type == AST_BLOCK) {
        for (i = 0; i < node->data.block.count; i++)
            compile_node(c, node->data.block.statements[i]);
    } else {
        compile_node(c, node);
    }
}

static void compile_node(SkyCompiler *c, SkyASTNode *node) {
    int i, slot, jump_false, jump_end, loop_start;
    if (!node) return;
    switch (node->type) {
        case AST_PROGRAM:
            for (i = 0; i < node->data.program.count; i++)
                compile_node(c, node->data.program.statements[i]);
            break;

        case AST_INT_LITERAL:
            emit_constant(c, SKY_INT(node->data.int_literal.value), node->line);
            break;

        case AST_FLOAT_LITERAL:
            emit_constant(c, SKY_FLOAT(node->data.float_literal.value), node->line);
            break;

        case AST_STRING_LITERAL:
            emit_constant(c, SKY_STRING(node->data.string_literal.value), node->line);
            break;

        case AST_BOOL_LITERAL:
            emit_byte(c, node->data.bool_literal.value ? OP_TRUE : OP_FALSE, node->line);
            break;

        case AST_NIL_LITERAL:
            emit_byte(c, OP_NIL, node->line);
            break;

        case AST_IDENTIFIER: {
            const char *name = node->data.identifier.name;
            slot = resolve_local(c, name);
            if (slot >= 0) {
                emit_bytes(c, OP_GET_LOCAL, (uint8_t)slot, node->line);
            } else {
                emit_bytes(c, OP_GET_GLOBAL,
                    (uint8_t)make_constant(c, SKY_STRING(name)), node->line);
            }
            break;
        }

        case AST_BINARY:
            compile_node(c, node->data.binary.left);
            compile_node(c, node->data.binary.right);
            switch (node->data.binary.op) {
                case TOKEN_PLUS:          emit_byte(c, OP_ADD, node->line); break;
                case TOKEN_MINUS:         emit_byte(c, OP_SUB, node->line); break;
                case TOKEN_STAR:          emit_byte(c, OP_MUL, node->line); break;
                case TOKEN_SLASH:         emit_byte(c, OP_DIV, node->line); break;
                case TOKEN_PERCENT:       emit_byte(c, OP_MOD, node->line); break;
                case TOKEN_EQUAL_EQUAL:   emit_byte(c, OP_EQUAL, node->line); break;
                case TOKEN_NOT_EQUAL:     emit_byte(c, OP_NOT_EQUAL, node->line); break;
                case TOKEN_LESS:          emit_byte(c, OP_LESS, node->line); break;
                case TOKEN_LESS_EQUAL:    emit_byte(c, OP_LESS_EQ, node->line); break;
                case TOKEN_GREATER:       emit_byte(c, OP_GREATER, node->line); break;
                case TOKEN_GREATER_EQUAL: emit_byte(c, OP_GREATER_EQ, node->line); break;
                case TOKEN_AND:           emit_byte(c, OP_AND, node->line); break;
                case TOKEN_OR:            emit_byte(c, OP_OR, node->line); break;
                default:
                    fprintf(stderr, "Compiler error: Unknown binary operator %d\n", node->data.binary.op);
                    c->had_error = true;
                    break;
            }
            break;

        case AST_UNARY:
            compile_node(c, node->data.unary.operand);
            switch (node->data.unary.op) {
                case TOKEN_MINUS: emit_byte(c, OP_NEGATE, node->line); break;
                case TOKEN_NOT:   emit_byte(c, OP_NOT, node->line); break;
                default:
                    fprintf(stderr, "Compiler error: Unknown unary operator\n");
                    c->had_error = true;
                    break;
            }
            break;

        case AST_CALL:
            compile_node(c, node->data.call.callee);
            for (i = 0; i < node->data.call.arg_count; i++)
                compile_node(c, node->data.call.args[i]);
            emit_bytes(c, OP_CALL, (uint8_t)node->data.call.arg_count, node->line);
            break;

        case AST_DOT:
            compile_node(c, node->data.dot.object);
            emit_bytes(c, OP_GET_FIELD,
                (uint8_t)make_constant(c, SKY_STRING(node->data.dot.field)), node->line);
            break;

        case AST_INDEX:
            compile_node(c, node->data.index_access.object);
            compile_node(c, node->data.index_access.index);
            emit_byte(c, OP_GET_INDEX, node->line);
            break;

        case AST_ARRAY_LITERAL:
            for (i = 0; i < node->data.array_literal.count; i++)
                compile_node(c, node->data.array_literal.elements[i]);
            emit_bytes(c, OP_ARRAY, (uint8_t)node->data.array_literal.count, node->line);
            break;

        case AST_ASSIGN:
            compile_node(c, node->data.assign.value);
            if (node->data.assign.target->type == AST_IDENTIFIER) {
                const char *name = node->data.assign.target->data.identifier.name;
                slot = resolve_local(c, name);
                if (slot >= 0) {
                    emit_bytes(c, OP_SET_LOCAL, (uint8_t)slot, node->line);
                } else {
                    emit_bytes(c, OP_SET_GLOBAL,
                        (uint8_t)make_constant(c, SKY_STRING(name)), node->line);
                }
            } else if (node->data.assign.target->type == AST_DOT) {
                compile_node(c, node->data.assign.target->data.dot.object);
                emit_bytes(c, OP_SET_FIELD,
                    (uint8_t)make_constant(c, SKY_STRING(node->data.assign.target->data.dot.field)),
                    node->line);
            } else if (node->data.assign.target->type == AST_INDEX) {
                compile_node(c, node->data.assign.target->data.index_access.object);
                compile_node(c, node->data.assign.target->data.index_access.index);
                emit_byte(c, OP_SET_INDEX, node->line);
            }
            break;

        case AST_LET:
            if (node->data.let.initializer) {
                compile_node(c, node->data.let.initializer);
            } else {
                emit_byte(c, OP_NIL, node->line);
            }
            if (c->scope_depth > 0) {
                add_local(c, node->data.let.name);
            } else {
                emit_bytes(c, OP_SET_GLOBAL,
                    (uint8_t)make_constant(c, SKY_STRING(node->data.let.name)), node->line);
            }
            break;

        case AST_IF:
            compile_node(c, node->data.if_stmt.condition);
            jump_false = emit_jump(c, OP_JUMP_IF_FALSE, node->line);
            emit_byte(c, OP_POP, node->line);
            compile_node(c, node->data.if_stmt.then_branch);
            if (node->data.if_stmt.else_branch) {
                jump_end = emit_jump(c, OP_JUMP, node->line);
                patch_jump(c, jump_false);
                emit_byte(c, OP_POP, node->line);
                compile_node(c, node->data.if_stmt.else_branch);
                patch_jump(c, jump_end);
            } else {
                patch_jump(c, jump_false);
                emit_byte(c, OP_POP, node->line);
            }
            break;

        case AST_WHILE:
            loop_start = c->chunk->code_count;
            compile_node(c, node->data.while_stmt.condition);
            jump_false = emit_jump(c, OP_JUMP_IF_FALSE, node->line);
            emit_byte(c, OP_POP, node->line);
            compile_block(c, node->data.while_stmt.body);
            {
                int back = c->chunk->code_count - loop_start + 3;
                emit_byte(c, OP_JUMP_BACK, node->line);
                emit_byte(c, (back >> 8) & 0xff, node->line);
                emit_byte(c, back & 0xff, node->line);
            }
            patch_jump(c, jump_false);
            emit_byte(c, OP_POP, node->line);
            break;

        case AST_FOR:
            begin_scope(c);
            /* Push initial value (start of range) */
            if (node->data.for_range.start)
                compile_node(c, node->data.for_range.start);
            else
                emit_constant(c, SKY_INT(0), node->line);
            slot = add_local(c, node->data.for_range.var_name);
            /* Loop start */
            loop_start = c->chunk->code_count;
            /* Compare: local < end */
            emit_bytes(c, OP_GET_LOCAL, (uint8_t)slot, node->line);
            compile_node(c, node->data.for_range.end);
            emit_byte(c, OP_LESS, node->line);
            jump_false = emit_jump(c, OP_JUMP_IF_FALSE, node->line);
            emit_byte(c, OP_POP, node->line);  /* pop condition */
            /* Body */
            compile_block(c, node->data.for_range.body);
            /* Increment: local = local + 1 */
            emit_bytes(c, OP_GET_LOCAL, (uint8_t)slot, node->line);
            emit_constant(c, SKY_INT(1), node->line);
            emit_byte(c, OP_ADD, node->line);
            emit_bytes(c, OP_SET_LOCAL, (uint8_t)slot, node->line);
            emit_byte(c, OP_POP, node->line);  /* pop set result */
            /* Jump back */
            {
                int back = c->chunk->code_count - loop_start + 3;
                emit_byte(c, OP_JUMP_BACK, node->line);
                emit_byte(c, (back >> 8) & 0xff, node->line);
                emit_byte(c, back & 0xff, node->line);
            }
            patch_jump(c, jump_false);
            emit_byte(c, OP_POP, node->line);  /* pop condition */
            end_scope(c, node->line);
            break;

        case AST_BLOCK:
            begin_scope(c);
            for (i = 0; i < node->data.block.count; i++)
                compile_node(c, node->data.block.statements[i]);
            end_scope(c, node->line);
            break;

        case AST_FUNCTION:
            /* Simplified: store function name as global with nil for now */
            emit_byte(c, OP_NIL, node->line);
            if (c->scope_depth == 0) {
                emit_bytes(c, OP_SET_GLOBAL,
                    (uint8_t)make_constant(c, SKY_STRING(node->data.function.name)), node->line);
            }
            break;

        case AST_RETURN:
            if (node->data.return_stmt.value) {
                compile_node(c, node->data.return_stmt.value);
            } else {
                emit_byte(c, OP_NIL, node->line);
            }
            emit_byte(c, OP_RETURN, node->line);
            break;

        case AST_PRINT:
            compile_node(c, node->data.print_stmt.value);
            emit_byte(c, OP_PRINT, node->line);
            break;

        case AST_EXPRESSION_STMT:
            compile_node(c, node->data.expr_stmt.expr);
            emit_byte(c, OP_POP, node->line);
            break;

        case AST_IMPORT:
        case AST_SERVER:
        case AST_ROUTE:
        case AST_RESPOND:
        case AST_SECURITY:
        case AST_SECURITY_RULE:
        case AST_CLASS:
        case AST_MAP_LITERAL:
        case AST_FOR_IN:
        case AST_BREAK:
        case AST_CONTINUE:
            /* Not yet implemented in compiler */
            break;

        default:
            break;
    }
}

void sky_compiler_init(SkyCompiler *compiler, SkyChunk *chunk) {
    if (!compiler || !chunk) return;
    compiler->chunk = chunk;
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    compiler->had_error = false;
}

bool sky_compiler_compile(SkyCompiler *compiler, SkyASTNode *ast) {
    if (!compiler || !ast) return false;
    compile_node(compiler, ast);
    return !compiler->had_error;
}

void sky_chunk_init(SkyChunk *chunk) {
    if (!chunk) return;
    chunk->code = NULL;
    chunk->code_count = 0;
    chunk->code_capacity = 0;
    chunk->lines = NULL;
    sky_value_array_init(&chunk->constants);
}

void sky_chunk_write(SkyChunk *chunk, uint8_t byte, int line) {
    if (chunk->code_count >= chunk->code_capacity) {
        int new_cap = chunk->code_capacity < 8 ? 8 : chunk->code_capacity * 2;
        chunk->code = (uint8_t*)realloc(chunk->code, new_cap);
        chunk->lines = (int*)realloc(chunk->lines, sizeof(int) * new_cap);
        chunk->code_capacity = new_cap;
    }
    chunk->code[chunk->code_count] = byte;
    chunk->lines[chunk->code_count] = line;
    chunk->code_count++;
}

void sky_chunk_free(SkyChunk *chunk) {
    if (!chunk) return;
    free(chunk->code);
    free(chunk->lines);
    sky_value_array_free(&chunk->constants);
    sky_chunk_init(chunk);
}

int sky_chunk_add_constant(SkyChunk *chunk, SkyValue value) {
    sky_value_array_write(&chunk->constants, value);
    return chunk->constants.count - 1;
}


