/* analyzer.c — Type checker implementation */
#include "analyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static void analyze_error(SkyAnalyzer *a, int line, const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "Error in %s at line %d: ", a->filename, line);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    a->error_count++;
    a->had_error = true;
}

static void analyze_node(SkyAnalyzer *a, SkyASTNode *node) {
    int i;
    if (!node) return;
    switch (node->type) {
        case AST_PROGRAM:
            for (i = 0; i < node->data.program.count; i++)
                analyze_node(a, node->data.program.statements[i]);
            break;
        case AST_BLOCK:
            for (i = 0; i < node->data.block.count; i++)
                analyze_node(a, node->data.block.statements[i]);
            break;
        case AST_LET:
            if (!node->data.let.name) {
                analyze_error(a, node->line, "Variable declaration missing name");
            }
            analyze_node(a, node->data.let.initializer);
            break;
        case AST_IF:
            analyze_node(a, node->data.if_stmt.condition);
            analyze_node(a, node->data.if_stmt.then_branch);
            analyze_node(a, node->data.if_stmt.else_branch);
            break;
        case AST_WHILE:
            analyze_node(a, node->data.while_stmt.condition);
            analyze_node(a, node->data.while_stmt.body);
            break;
        case AST_FOR:
            analyze_node(a, node->data.for_range.start);
            analyze_node(a, node->data.for_range.end);
            analyze_node(a, node->data.for_range.body);
            break;
        case AST_FOR_IN:
            analyze_node(a, node->data.for_each.iterable);
            analyze_node(a, node->data.for_each.body);
            break;
        case AST_FUNCTION:
            if (!node->data.function.name) {
                analyze_error(a, node->line, "Function missing name");
            }
            analyze_node(a, node->data.function.body);
            break;
        case AST_RETURN:
            analyze_node(a, node->data.return_stmt.value);
            break;
        case AST_BINARY:
            analyze_node(a, node->data.binary.left);
            analyze_node(a, node->data.binary.right);
            break;
        case AST_UNARY:
            analyze_node(a, node->data.unary.operand);
            break;
        case AST_CALL:
            analyze_node(a, node->data.call.callee);
            for (i = 0; i < node->data.call.arg_count; i++)
                analyze_node(a, node->data.call.args[i]);
            break;
        case AST_ASSIGN:
            analyze_node(a, node->data.assign.target);
            analyze_node(a, node->data.assign.value);
            break;
        case AST_CLASS:
            if (!node->data.class_def.name) {
                analyze_error(a, node->line, "Class missing name");
            }
            for (i = 0; i < node->data.class_def.member_count; i++)
                analyze_node(a, node->data.class_def.members[i]);
            break;
        case AST_SERVER:
            for (i = 0; i < node->data.server.route_count; i++)
                analyze_node(a, node->data.server.routes[i]);
            break;
        case AST_ROUTE:
            analyze_node(a, node->data.route.body);
            break;
        case AST_RESPOND:
            analyze_node(a, node->data.respond.status);
            analyze_node(a, node->data.respond.body);
            break;
        case AST_PRINT:
            analyze_node(a, node->data.print_stmt.value);
            break;
        case AST_EXPRESSION_STMT:
            analyze_node(a, node->data.expr_stmt.expr);
            break;
        default:
            break;
    }
}

void sky_analyzer_init(SkyAnalyzer *analyzer, const char *filename) {
    if (!analyzer) return;
    analyzer->filename = filename;
    analyzer->error_count = 0;
    analyzer->had_error = false;
}

bool sky_analyzer_analyze(SkyAnalyzer *analyzer, SkyASTNode *ast) {
    if (!analyzer || !ast) return false;
    analyze_node(analyzer, ast);
    if (analyzer->had_error) {
        fprintf(stderr, "\n%d error(s) found.\n", analyzer->error_count);
    }
    return !analyzer->had_error;
}
