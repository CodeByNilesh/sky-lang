/* ast.h — Abstract Syntax Tree node definitions */
#ifndef SKY_AST_H
#define SKY_AST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    AST_PROGRAM,
    AST_INT_LITERAL,
    AST_FLOAT_LITERAL,
    AST_STRING_LITERAL,
    AST_BOOL_LITERAL,
    AST_NIL_LITERAL,
    AST_IDENTIFIER,
    AST_BINARY,
    AST_UNARY,
    AST_CALL,
    AST_DOT,
    AST_INDEX,
    AST_ARRAY_LITERAL,
    AST_MAP_LITERAL,
    AST_ASSIGN,
    AST_LET,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_FOR_IN,
    AST_BLOCK,
    AST_FUNCTION,
    AST_RETURN,
    AST_PRINT,
    AST_CLASS,
    AST_SERVER,
    AST_ROUTE,
    AST_RESPOND,
    AST_SECURITY,
    AST_SECURITY_RULE,
    AST_IMPORT,
    AST_BREAK,
    AST_CONTINUE,
    AST_EXPRESSION_STMT
} SkyASTType;

typedef struct SkyASTNode SkyASTNode;

struct SkyASTNode {
    SkyASTType type;
    int line;
    union {
        /* program */
        struct {
            SkyASTNode **statements;
            int count;
            int capacity;
        } program;

        /* int literal */
        struct { int64_t value; } int_literal;

        /* float literal */
        struct { double value; } float_literal;

        /* string literal */
        struct { char *value; } string_literal;

        /* bool literal */
        struct { bool value; } bool_literal;

        /* identifier */
        struct { char *name; } identifier;

        /* binary */
        struct {
            int op;
            SkyASTNode *left;
            SkyASTNode *right;
        } binary;

        /* unary */
        struct {
            int op;
            SkyASTNode *operand;
        } unary;

        /* call */
        struct {
            SkyASTNode *callee;
            SkyASTNode **args;
            int arg_count;
        } call;

        /* dot access */
        struct {
            SkyASTNode *object;
            char *field;
        } dot;

        /* index access */
        struct {
            SkyASTNode *object;
            SkyASTNode *index;
        } index_access;

        /* array literal */
        struct {
            SkyASTNode **elements;
            int count;
        } array_literal;

        /* map literal */
        struct {
            SkyASTNode **keys;
            SkyASTNode **values;
            int count;
        } map_literal;

        /* assign */
        struct {
            SkyASTNode *target;
            SkyASTNode *value;
        } assign;

        /* let */
        struct {
            char *name;
            char *type_name;
            SkyASTNode *initializer;
        } let;

        /* if */
        struct {
            SkyASTNode *condition;
            SkyASTNode *then_branch;
            SkyASTNode *else_branch;
        } if_stmt;

        /* while */
        struct {
            SkyASTNode *condition;
            SkyASTNode *body;
        } while_stmt;

        /* for range: for i in start..end */
        struct {
            char *var_name;
            SkyASTNode *start;
            SkyASTNode *end;
            SkyASTNode *body;
        } for_range;

        /* for each: for item in collection */
        struct {
            char *var_name;
            SkyASTNode *iterable;
            SkyASTNode *body;
        } for_each;

        /* block */
        struct {
            SkyASTNode **statements;
            int count;
            int capacity;
        } block;

        /* function */
        struct {
            char *name;
            char **param_names;
            char **param_types;
            int param_count;
            char *return_type;
            SkyASTNode *body;
            bool is_async;
        } function;

        /* return */
        struct {
            SkyASTNode *value;
        } return_stmt;

        /* print */
        struct {
            SkyASTNode *value;
        } print_stmt;

        /* class */
        struct {
            char *name;
            SkyASTNode **members;
            int member_count;
        } class_def;

        /* server */
        struct {
            char *name;
            int port;
            SkyASTNode **routes;
            int route_count;
        } server;

        /* route */
        struct {
            char *method;
            char *path;
            SkyASTNode *body;
            char **middleware;
            int middleware_count;
        } route;

        /* respond */
        struct {
            SkyASTNode *status;
            SkyASTNode *body;
        } respond;

        /* security */
        struct {
            SkyASTNode **rules;
            int rule_count;
        } security;

        /* security rule */
        struct {
            char *event;
            SkyASTNode **actions;
            int action_count;
        } security_rule;

        /* import */
        struct {
            char *module_name;
        } import_stmt;

        /* expression statement */
        struct {
            SkyASTNode *expr;
        } expr_stmt;
    } data;
};

SkyASTNode* sky_ast_new(SkyASTType type, int line);
void sky_ast_free(SkyASTNode *node);
void sky_ast_program_add(SkyASTNode *program, SkyASTNode *stmt);
void sky_ast_block_add(SkyASTNode *block, SkyASTNode *stmt);

#endif
