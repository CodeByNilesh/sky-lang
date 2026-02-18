/* ast.c — AST utilities implementation */
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SkyASTNode* sky_ast_new(SkyASTType type, int line) {
    SkyASTNode *node = (SkyASTNode*)calloc(1, sizeof(SkyASTNode));
    if (!node) return NULL;
    node->type = type;
    node->line = line;
    return node;
}

void sky_ast_program_add(SkyASTNode *program, SkyASTNode *stmt) {
    if (!program || !stmt) return;
    if (program->data.program.count >= program->data.program.capacity) {
        int new_cap = program->data.program.capacity < 8 ? 8 : program->data.program.capacity * 2;
        program->data.program.statements = (SkyASTNode**)realloc(
            program->data.program.statements, sizeof(SkyASTNode*) * new_cap);
        program->data.program.capacity = new_cap;
    }
    program->data.program.statements[program->data.program.count++] = stmt;
}

void sky_ast_block_add(SkyASTNode *block, SkyASTNode *stmt) {
    if (!block || !stmt) return;
    if (block->data.block.count >= block->data.block.capacity) {
        int new_cap = block->data.block.capacity < 8 ? 8 : block->data.block.capacity * 2;
        block->data.block.statements = (SkyASTNode**)realloc(
            block->data.block.statements, sizeof(SkyASTNode*) * new_cap);
        block->data.block.capacity = new_cap;
    }
    block->data.block.statements[block->data.block.count++] = stmt;
}

void sky_ast_free(SkyASTNode *node) {
    int i;
    if (!node) return;
    switch (node->type) {
        case AST_PROGRAM:
            for (i = 0; i < node->data.program.count; i++)
                sky_ast_free(node->data.program.statements[i]);
            free(node->data.program.statements);
            break;
        case AST_STRING_LITERAL:
            free(node->data.string_literal.value);
            break;
        case AST_IDENTIFIER:
            free(node->data.identifier.name);
            break;
        case AST_BINARY:
            sky_ast_free(node->data.binary.left);
            sky_ast_free(node->data.binary.right);
            break;
        case AST_UNARY:
            sky_ast_free(node->data.unary.operand);
            break;
        case AST_CALL:
            sky_ast_free(node->data.call.callee);
            for (i = 0; i < node->data.call.arg_count; i++)
                sky_ast_free(node->data.call.args[i]);
            free(node->data.call.args);
            break;
        case AST_DOT:
            sky_ast_free(node->data.dot.object);
            free(node->data.dot.field);
            break;
        case AST_INDEX:
            sky_ast_free(node->data.index_access.object);
            sky_ast_free(node->data.index_access.index);
            break;
        case AST_ARRAY_LITERAL:
            for (i = 0; i < node->data.array_literal.count; i++)
                sky_ast_free(node->data.array_literal.elements[i]);
            free(node->data.array_literal.elements);
            break;
        case AST_MAP_LITERAL:
            for (i = 0; i < node->data.map_literal.count; i++) {
                sky_ast_free(node->data.map_literal.keys[i]);
                sky_ast_free(node->data.map_literal.values[i]);
            }
            free(node->data.map_literal.keys);
            free(node->data.map_literal.values);
            break;
        case AST_ASSIGN:
            sky_ast_free(node->data.assign.target);
            sky_ast_free(node->data.assign.value);
            break;
        case AST_LET:
            free(node->data.let.name);
            free(node->data.let.type_name);
            sky_ast_free(node->data.let.initializer);
            break;
        case AST_IF:
            sky_ast_free(node->data.if_stmt.condition);
            sky_ast_free(node->data.if_stmt.then_branch);
            sky_ast_free(node->data.if_stmt.else_branch);
            break;
        case AST_WHILE:
            sky_ast_free(node->data.while_stmt.condition);
            sky_ast_free(node->data.while_stmt.body);
            break;
        case AST_FOR:
            free(node->data.for_range.var_name);
            sky_ast_free(node->data.for_range.start);
            sky_ast_free(node->data.for_range.end);
            sky_ast_free(node->data.for_range.body);
            break;
        case AST_FOR_IN:
            free(node->data.for_each.var_name);
            sky_ast_free(node->data.for_each.iterable);
            sky_ast_free(node->data.for_each.body);
            break;
        case AST_BLOCK:
            for (i = 0; i < node->data.block.count; i++)
                sky_ast_free(node->data.block.statements[i]);
            free(node->data.block.statements);
            break;
        case AST_FUNCTION:
            free(node->data.function.name);
            for (i = 0; i < node->data.function.param_count; i++) {
                free(node->data.function.param_names[i]);
                free(node->data.function.param_types[i]);
            }
            free(node->data.function.param_names);
            free(node->data.function.param_types);
            free(node->data.function.return_type);
            sky_ast_free(node->data.function.body);
            break;
        case AST_RETURN:
            sky_ast_free(node->data.return_stmt.value);
            break;
        case AST_PRINT:
            sky_ast_free(node->data.print_stmt.value);
            break;
        case AST_CLASS:
            free(node->data.class_def.name);
            for (i = 0; i < node->data.class_def.member_count; i++)
                sky_ast_free(node->data.class_def.members[i]);
            free(node->data.class_def.members);
            break;
        case AST_SERVER:
            free(node->data.server.name);
            for (i = 0; i < node->data.server.route_count; i++)
                sky_ast_free(node->data.server.routes[i]);
            free(node->data.server.routes);
            break;
        case AST_ROUTE:
            free(node->data.route.method);
            free(node->data.route.path);
            sky_ast_free(node->data.route.body);
            for (i = 0; i < node->data.route.middleware_count; i++)
                free(node->data.route.middleware[i]);
            free(node->data.route.middleware);
            break;
        case AST_RESPOND:
            sky_ast_free(node->data.respond.status);
            sky_ast_free(node->data.respond.body);
            break;
        case AST_SECURITY:
            for (i = 0; i < node->data.security.rule_count; i++)
                sky_ast_free(node->data.security.rules[i]);
            free(node->data.security.rules);
            break;
        case AST_SECURITY_RULE:
            free(node->data.security_rule.event);
            for (i = 0; i < node->data.security_rule.action_count; i++)
                sky_ast_free(node->data.security_rule.actions[i]);
            free(node->data.security_rule.actions);
            break;
        case AST_IMPORT:
            free(node->data.import_stmt.module_name);
            break;
        case AST_EXPRESSION_STMT:
            sky_ast_free(node->data.expr_stmt.expr);
            break;
        default:
            break;
    }
    free(node);
}
