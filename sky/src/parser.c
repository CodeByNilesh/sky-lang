/* parser.c — Parser implementation */
#include "parser.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void advance(SkyParser *p) {
    p->previous = p->current;
    p->current = sky_lexer_next(p->lexer);
}

static bool check(SkyParser *p, SkyTokenType type) {
    return p->current.type == type;
}

static bool match(SkyParser *p, SkyTokenType type) {
    if (!check(p, type)) return false;
    advance(p);
    return true;
}

static void error_at(SkyParser *p, SkyToken *tok, const char *msg) {
    if (p->panic_mode) return;
    p->panic_mode = true;
    p->had_error = true;
    fprintf(stderr, "Error at line %d", tok->line);
    if (tok->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else {
        fprintf(stderr, " at '%.*s'", tok->length, tok->start);
    }
    fprintf(stderr, ": %s\n", msg);
}

static void consume(SkyParser *p, SkyTokenType type, const char *msg) {
    if (check(p, type)) { advance(p); return; }
    error_at(p, &p->current, msg);
}

static char* copy_token_text(SkyToken *tok) {
    char *s = (char*)malloc(tok->length + 1);
    memcpy(s, tok->start, tok->length);
    s[tok->length] = '\0';
    return s;
}

/* Forward declarations */
static SkyASTNode* parse_expression(SkyParser *p);
static SkyASTNode* parse_statement(SkyParser *p);
static SkyASTNode* parse_block(SkyParser *p);

/* ── Expression parsing ────────────────────────── */

static SkyASTNode* parse_primary(SkyParser *p) {
    if (match(p, TOKEN_INT_LITERAL)) {
        SkyASTNode *n = sky_ast_new(AST_INT_LITERAL, p->previous.line);
        n->data.int_literal.value = strtoll(p->previous.start, NULL, 10);
        return n;
    }
    if (match(p, TOKEN_FLOAT_LITERAL)) {
        SkyASTNode *n = sky_ast_new(AST_FLOAT_LITERAL, p->previous.line);
        n->data.float_literal.value = strtod(p->previous.start, NULL);
        return n;
    }
    if (match(p, TOKEN_STRING_LITERAL)) {
        SkyASTNode *n = sky_ast_new(AST_STRING_LITERAL, p->previous.line);
        n->data.string_literal.value = copy_token_text(&p->previous);
        return n;
    }
    if (match(p, TOKEN_TRUE)) {
        SkyASTNode *n = sky_ast_new(AST_BOOL_LITERAL, p->previous.line);
        n->data.bool_literal.value = true;
        return n;
    }
    if (match(p, TOKEN_FALSE)) {
        SkyASTNode *n = sky_ast_new(AST_BOOL_LITERAL, p->previous.line);
        n->data.bool_literal.value = false;
        return n;
    }
    if (match(p, TOKEN_NIL)) {
        return sky_ast_new(AST_NIL_LITERAL, p->previous.line);
    }
    if (match(p, TOKEN_IDENTIFIER)) {
        SkyASTNode *n = sky_ast_new(AST_IDENTIFIER, p->previous.line);
        n->data.identifier.name = copy_token_text(&p->previous);
        return n;
    }
    if (match(p, TOKEN_LPAREN)) {
        SkyASTNode *expr = parse_expression(p);
        consume(p, TOKEN_RPAREN, "Expected ')'");
        return expr;
    }
    if (match(p, TOKEN_LBRACKET)) {
        SkyASTNode *arr = sky_ast_new(AST_ARRAY_LITERAL, p->previous.line);
        int cap = 8;
        arr->data.array_literal.elements = (SkyASTNode**)malloc(sizeof(SkyASTNode*) * cap);
        arr->data.array_literal.count = 0;
        if (!check(p, TOKEN_RBRACKET)) {
            do {
                if (arr->data.array_literal.count >= cap) {
                    cap *= 2;
                    arr->data.array_literal.elements = (SkyASTNode**)realloc(
                        arr->data.array_literal.elements, sizeof(SkyASTNode*) * cap);
                }
                arr->data.array_literal.elements[arr->data.array_literal.count++] = parse_expression(p);
            } while (match(p, TOKEN_COMMA));
        }
        consume(p, TOKEN_RBRACKET, "Expected ']'");
        return arr;
    }
    error_at(p, &p->current, "Expected expression");
    advance(p);
    return sky_ast_new(AST_NIL_LITERAL, p->current.line);
}

static SkyASTNode* parse_call(SkyParser *p) {
    SkyASTNode *expr = parse_primary(p);
    while (1) {
        if (match(p, TOKEN_LPAREN)) {
            SkyASTNode *call = sky_ast_new(AST_CALL, p->previous.line);
            call->data.call.callee = expr;
            int cap = 8;
            call->data.call.args = (SkyASTNode**)malloc(sizeof(SkyASTNode*) * cap);
            call->data.call.arg_count = 0;
            if (!check(p, TOKEN_RPAREN)) {
                do {
                    if (call->data.call.arg_count >= cap) {
                        cap *= 2;
                        call->data.call.args = (SkyASTNode**)realloc(
                            call->data.call.args, sizeof(SkyASTNode*) * cap);
                    }
                    call->data.call.args[call->data.call.arg_count++] = parse_expression(p);
                } while (match(p, TOKEN_COMMA));
            }
            consume(p, TOKEN_RPAREN, "Expected ')'");
            expr = call;
        } else if (match(p, TOKEN_DOT)) {
            consume(p, TOKEN_IDENTIFIER, "Expected field name");
            SkyASTNode *dot = sky_ast_new(AST_DOT, p->previous.line);
            dot->data.dot.object = expr;
            dot->data.dot.field = copy_token_text(&p->previous);
            expr = dot;
        } else if (match(p, TOKEN_LBRACKET)) {
            SkyASTNode *idx = sky_ast_new(AST_INDEX, p->previous.line);
            idx->data.index_access.object = expr;
            idx->data.index_access.index = parse_expression(p);
            consume(p, TOKEN_RBRACKET, "Expected ']'");
            expr = idx;
        } else {
            break;
        }
    }
    return expr;
}

static SkyASTNode* parse_unary(SkyParser *p) {
    if (match(p, TOKEN_MINUS) || match(p, TOKEN_NOT)) {
        int op = p->previous.type;
        int line = p->previous.line;
        SkyASTNode *operand = parse_unary(p);
        SkyASTNode *n = sky_ast_new(AST_UNARY, line);
        n->data.unary.op = op;
        n->data.unary.operand = operand;
        return n;
    }
    return parse_call(p);
}

static SkyASTNode* parse_factor(SkyParser *p) {
    SkyASTNode *left = parse_unary(p);
    while (match(p, TOKEN_STAR) || match(p, TOKEN_SLASH) || match(p, TOKEN_PERCENT)) {
        int op = p->previous.type;
        int line = p->previous.line;
        SkyASTNode *right = parse_unary(p);
        SkyASTNode *bin = sky_ast_new(AST_BINARY, line);
        bin->data.binary.op = op;
        bin->data.binary.left = left;
        bin->data.binary.right = right;
        left = bin;
    }
    return left;
}

static SkyASTNode* parse_term(SkyParser *p) {
    SkyASTNode *left = parse_factor(p);
    while (match(p, TOKEN_PLUS) || match(p, TOKEN_MINUS)) {
        int op = p->previous.type;
        int line = p->previous.line;
        SkyASTNode *right = parse_factor(p);
        SkyASTNode *bin = sky_ast_new(AST_BINARY, line);
        bin->data.binary.op = op;
        bin->data.binary.left = left;
        bin->data.binary.right = right;
        left = bin;
    }
    return left;
}

static SkyASTNode* parse_comparison(SkyParser *p) {
    SkyASTNode *left = parse_term(p);
    while (match(p, TOKEN_LESS) || match(p, TOKEN_LESS_EQUAL) ||
           match(p, TOKEN_GREATER) || match(p, TOKEN_GREATER_EQUAL)) {
        int op = p->previous.type;
        int line = p->previous.line;
        SkyASTNode *right = parse_term(p);
        SkyASTNode *bin = sky_ast_new(AST_BINARY, line);
        bin->data.binary.op = op;
        bin->data.binary.left = left;
        bin->data.binary.right = right;
        left = bin;
    }
    return left;
}

static SkyASTNode* parse_equality(SkyParser *p) {
    SkyASTNode *left = parse_comparison(p);
    while (match(p, TOKEN_EQUAL_EQUAL) || match(p, TOKEN_NOT_EQUAL)) {
        int op = p->previous.type;
        int line = p->previous.line;
        SkyASTNode *right = parse_comparison(p);
        SkyASTNode *bin = sky_ast_new(AST_BINARY, line);
        bin->data.binary.op = op;
        bin->data.binary.left = left;
        bin->data.binary.right = right;
        left = bin;
    }
    return left;
}

static SkyASTNode* parse_logic_and(SkyParser *p) {
    SkyASTNode *left = parse_equality(p);
    while (match(p, TOKEN_AND)) {
        int line = p->previous.line;
        SkyASTNode *right = parse_equality(p);
        SkyASTNode *bin = sky_ast_new(AST_BINARY, line);
        bin->data.binary.op = TOKEN_AND;
        bin->data.binary.left = left;
        bin->data.binary.right = right;
        left = bin;
    }
    return left;
}

static SkyASTNode* parse_logic_or(SkyParser *p) {
    SkyASTNode *left = parse_logic_and(p);
    while (match(p, TOKEN_OR)) {
        int line = p->previous.line;
        SkyASTNode *right = parse_logic_and(p);
        SkyASTNode *bin = sky_ast_new(AST_BINARY, line);
        bin->data.binary.op = TOKEN_OR;
        bin->data.binary.left = left;
        bin->data.binary.right = right;
        left = bin;
    }
    return left;
}

static SkyASTNode* parse_assignment(SkyParser *p) {
    SkyASTNode *left = parse_logic_or(p);
    if (match(p, TOKEN_ASSIGN)) {
        int line = p->previous.line;
        SkyASTNode *value = parse_assignment(p);
        SkyASTNode *assign = sky_ast_new(AST_ASSIGN, line);
        assign->data.assign.target = left;
        assign->data.assign.value = value;
        return assign;
    }
    return left;
}

static SkyASTNode* parse_expression(SkyParser *p) {
    return parse_assignment(p);
}

/* ── Statement parsing ─────────────────────────── */

static SkyASTNode* parse_block(SkyParser *p) {
    SkyASTNode *block = sky_ast_new(AST_BLOCK, p->current.line);
    consume(p, TOKEN_LBRACE, "Expected '{'");
    while (!check(p, TOKEN_RBRACE) && !check(p, TOKEN_EOF)) {
        SkyASTNode *stmt = parse_statement(p);
        if (stmt) sky_ast_block_add(block, stmt);
    }
    consume(p, TOKEN_RBRACE, "Expected '}'");
    return block;
}

static SkyASTNode* parse_let(SkyParser *p) {
    int line = p->previous.line;
    consume(p, TOKEN_IDENTIFIER, "Expected variable name");
    SkyASTNode *node = sky_ast_new(AST_LET, line);
    node->data.let.name = copy_token_text(&p->previous);
    node->data.let.type_name = NULL;
    node->data.let.initializer = NULL;
    if (match(p, TOKEN_ASSIGN)) {
        node->data.let.initializer = parse_expression(p);
    }
    return node;
}

static SkyASTNode* parse_if(SkyParser *p) {
    int line = p->previous.line;
    SkyASTNode *node = sky_ast_new(AST_IF, line);
    node->data.if_stmt.condition = parse_expression(p);
    node->data.if_stmt.then_branch = parse_block(p);
    node->data.if_stmt.else_branch = NULL;
    if (match(p, TOKEN_ELSE)) {
        if (check(p, TOKEN_IF)) {
            advance(p);
            node->data.if_stmt.else_branch = parse_if(p);
        } else {
            node->data.if_stmt.else_branch = parse_block(p);
        }
    }
    return node;
}

static SkyASTNode* parse_while(SkyParser *p) {
    int line = p->previous.line;
    SkyASTNode *node = sky_ast_new(AST_WHILE, line);
    node->data.while_stmt.condition = parse_expression(p);
    node->data.while_stmt.body = parse_block(p);
    return node;
}

static SkyASTNode* parse_for(SkyParser *p) {
    int line = p->previous.line;
    consume(p, TOKEN_IDENTIFIER, "Expected loop variable");
    char *var_name = copy_token_text(&p->previous);
    consume(p, TOKEN_IN, "Expected 'in'");
    SkyASTNode *iter = parse_expression(p);
    if (match(p, TOKEN_DOT_DOT)) {
        SkyASTNode *end = parse_expression(p);
        SkyASTNode *node = sky_ast_new(AST_FOR, line);
        node->data.for_range.var_name = var_name;
        node->data.for_range.start = iter;
        node->data.for_range.end = end;
        node->data.for_range.body = parse_block(p);
        return node;
    } else {
        SkyASTNode *node = sky_ast_new(AST_FOR_IN, line);
        node->data.for_each.var_name = var_name;
        node->data.for_each.iterable = iter;
        node->data.for_each.body = parse_block(p);
        return node;
    }
}

static SkyASTNode* parse_function(SkyParser *p) {
    int line = p->previous.line;
    consume(p, TOKEN_IDENTIFIER, "Expected function name");
    SkyASTNode *node = sky_ast_new(AST_FUNCTION, line);
    node->data.function.name = copy_token_text(&p->previous);
    node->data.function.is_async = false;
    consume(p, TOKEN_LPAREN, "Expected '('");
    int cap = 8;
    node->data.function.param_names = (char**)malloc(sizeof(char*) * cap);
    node->data.function.param_types = (char**)malloc(sizeof(char*) * cap);
    node->data.function.param_count = 0;
    if (!check(p, TOKEN_RPAREN)) {
        do {
            consume(p, TOKEN_IDENTIFIER, "Expected parameter name");
            char *pname = copy_token_text(&p->previous);
            char *ptype = NULL;
            if (check(p, TOKEN_IDENTIFIER)) {
                advance(p);
                ptype = copy_token_text(&p->previous);
            }
            if (node->data.function.param_count >= cap) {
                cap *= 2;
                node->data.function.param_names = (char**)realloc(
                    node->data.function.param_names, sizeof(char*) * cap);
                node->data.function.param_types = (char**)realloc(
                    node->data.function.param_types, sizeof(char*) * cap);
            }
            node->data.function.param_names[node->data.function.param_count] = pname;
            node->data.function.param_types[node->data.function.param_count] = ptype;
            node->data.function.param_count++;
        } while (match(p, TOKEN_COMMA));
    }
    consume(p, TOKEN_RPAREN, "Expected ')'");
    node->data.function.return_type = NULL;
    if (check(p, TOKEN_IDENTIFIER)) {
        advance(p);
        node->data.function.return_type = copy_token_text(&p->previous);
    }
    node->data.function.body = parse_block(p);
    return node;
}

static SkyASTNode* parse_return(SkyParser *p) {
    int line = p->previous.line;
    SkyASTNode *node = sky_ast_new(AST_RETURN, line);
    node->data.return_stmt.value = NULL;
    if (!check(p, TOKEN_RBRACE) && !check(p, TOKEN_EOF)) {
        node->data.return_stmt.value = parse_expression(p);
    }
    return node;
}

static SkyASTNode* parse_import(SkyParser *p) {
    int line = p->previous.line;
    consume(p, TOKEN_IDENTIFIER, "Expected module name");
    SkyASTNode *node = sky_ast_new(AST_IMPORT, line);
    node->data.import_stmt.module_name = copy_token_text(&p->previous);
    return node;
}

static SkyASTNode* parse_class(SkyParser *p) {
    int line = p->previous.line;
    consume(p, TOKEN_IDENTIFIER, "Expected class name");
    SkyASTNode *node = sky_ast_new(AST_CLASS, line);
    node->data.class_def.name = copy_token_text(&p->previous);
    node->data.class_def.members = NULL;
    node->data.class_def.member_count = 0;
    consume(p, TOKEN_LBRACE, "Expected '{'");
    int cap = 8;
    node->data.class_def.members = (SkyASTNode**)malloc(sizeof(SkyASTNode*) * cap);
    while (!check(p, TOKEN_RBRACE) && !check(p, TOKEN_EOF)) {
        if (match(p, TOKEN_FN)) {
            SkyASTNode *fn = parse_function(p);
            if (node->data.class_def.member_count >= cap) {
                cap *= 2;
                node->data.class_def.members = (SkyASTNode**)realloc(
                    node->data.class_def.members, sizeof(SkyASTNode*) * cap);
            }
            node->data.class_def.members[node->data.class_def.member_count++] = fn;
        } else if (check(p, TOKEN_IDENTIFIER)) {
            advance(p);
            if (check(p, TOKEN_IDENTIFIER)) {
                advance(p);
            }
        } else {
            advance(p);
        }
    }
    consume(p, TOKEN_RBRACE, "Expected '}'");
    return node;
}

static SkyASTNode* parse_server(SkyParser *p) {
    int line = p->previous.line;
    consume(p, TOKEN_IDENTIFIER, "Expected server name");
    SkyASTNode *node = sky_ast_new(AST_SERVER, line);
    node->data.server.name = copy_token_text(&p->previous);
    node->data.server.port = 8080;
    if (match(p, TOKEN_ON)) {
        if (match(p, TOKEN_INT_LITERAL)) {
            node->data.server.port = (int)strtol(p->previous.start, NULL, 10);
        }
    }
    consume(p, TOKEN_LBRACE, "Expected '{'");
    int cap = 16;
    node->data.server.routes = (SkyASTNode**)malloc(sizeof(SkyASTNode*) * cap);
    node->data.server.route_count = 0;
    while (!check(p, TOKEN_RBRACE) && !check(p, TOKEN_EOF)) {
        if (match(p, TOKEN_ROUTE)) {
            consume(p, TOKEN_IDENTIFIER, "Expected HTTP method");
            SkyASTNode *route = sky_ast_new(AST_ROUTE, p->previous.line);
            route->data.route.method = copy_token_text(&p->previous);
            consume(p, TOKEN_STRING_LITERAL, "Expected route path");
            route->data.route.path = copy_token_text(&p->previous);
            route->data.route.middleware = NULL;
            route->data.route.middleware_count = 0;
            if (match(p, TOKEN_LBRACKET)) {
                consume(p, TOKEN_IDENTIFIER, "Expected middleware name");
                route->data.route.middleware = (char**)malloc(sizeof(char*));
                route->data.route.middleware[0] = copy_token_text(&p->previous);
                route->data.route.middleware_count = 1;
                consume(p, TOKEN_RBRACKET, "Expected ']'");
            }
            route->data.route.body = parse_block(p);
            if (node->data.server.route_count >= cap) {
                cap *= 2;
                node->data.server.routes = (SkyASTNode**)realloc(
                    node->data.server.routes, sizeof(SkyASTNode*) * cap);
            }
            node->data.server.routes[node->data.server.route_count++] = route;
        } else {
            advance(p);
        }
    }
    consume(p, TOKEN_RBRACE, "Expected '}'");
    return node;
}

static SkyASTNode* parse_statement(SkyParser *p) {
    if (match(p, TOKEN_LET)) return parse_let(p);
    if (match(p, TOKEN_IF)) return parse_if(p);
    if (match(p, TOKEN_WHILE)) return parse_while(p);
    if (match(p, TOKEN_FOR)) return parse_for(p);
    if (match(p, TOKEN_FN)) return parse_function(p);
    if (match(p, TOKEN_RETURN)) return parse_return(p);
    if (match(p, TOKEN_IMPORT)) return parse_import(p);
    if (match(p, TOKEN_CLASS)) return parse_class(p);
    if (match(p, TOKEN_SERVER)) return parse_server(p);
    if (match(p, TOKEN_PRINT)) {
        int line = p->previous.line;
        consume(p, TOKEN_LPAREN, "Expected '('");
        SkyASTNode *node = sky_ast_new(AST_PRINT, line);
        node->data.print_stmt.value = parse_expression(p);
        consume(p, TOKEN_RPAREN, "Expected ')'");
        return node;
    }
    if (match(p, TOKEN_RESPOND)) {
        int line = p->previous.line;
        SkyASTNode *node = sky_ast_new(AST_RESPOND, line);
        node->data.respond.status = parse_expression(p);
        node->data.respond.body = parse_expression(p);
        return node;
    }
    /* Expression statement */
    SkyASTNode *expr = parse_expression(p);
    SkyASTNode *stmt = sky_ast_new(AST_EXPRESSION_STMT, expr->line);
    stmt->data.expr_stmt.expr = expr;
    return stmt;
}

void sky_parser_init(SkyParser *parser, SkyLexer *lexer) {
    if (!parser || !lexer) return;
    parser->lexer = lexer;
    parser->had_error = false;
    parser->panic_mode = false;
    advance(parser);
}

SkyASTNode* sky_parser_parse(SkyParser *parser) {
    SkyASTNode *program = sky_ast_new(AST_PROGRAM, 1);
    while (!check(parser, TOKEN_EOF)) {
        SkyASTNode *stmt = parse_statement(parser);
        if (stmt) sky_ast_program_add(program, stmt);
        if (parser->panic_mode) {
            parser->panic_mode = false;
            while (!check(parser, TOKEN_EOF) &&
                   !check(parser, TOKEN_LET) &&
                   !check(parser, TOKEN_FN) &&
                   !check(parser, TOKEN_CLASS) &&
                   !check(parser, TOKEN_IF) &&
                   !check(parser, TOKEN_FOR) &&
                   !check(parser, TOKEN_WHILE) &&
                   !check(parser, TOKEN_RETURN) &&
                   !check(parser, TOKEN_IMPORT) &&
                   !check(parser, TOKEN_SERVER)) {
                advance(parser);
            }
        }
    }
    return parser->had_error ? NULL : program;
}

bool sky_parser_had_error(const SkyParser *parser) {
    if (!parser) return true;
    return parser->had_error;
}
