/* tests/test_parser.c — Parser test suite */
#include "../src/lexer.h"
#include "../src/parser.h"
#include "../src/ast.h"
#include <stdio.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  TEST: %-40s ", name);
#define PASS() do { printf("✓ PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("✗ FAIL: %s\n", msg); tests_failed++; } while(0)

static void test_let_statement(void) {
    TEST("Parse let statement");

    SkyLexer lexer;
    sky_lexer_init(&lexer, "let x = 42", "test");

    SkyParser parser;
    sky_parser_init(&parser, &lexer);

    SkyASTNode *program = sky_parser_parse(&parser);

    if (!program) { FAIL("null AST"); return; }
    if (program->type != AST_PROGRAM) { FAIL("not a program"); sky_ast_free(program); return; }
    if (program->data.program.count < 1) { FAIL("no statements"); sky_ast_free(program); return; }

    SkyASTNode *stmt = program->data.program.statements[0];
    if (stmt->type != AST_LET) { FAIL("not a let"); sky_ast_free(program); return; }

    sky_ast_free(program);
    PASS();
}

static void test_function_decl(void) {
    TEST("Parse function declaration");

    SkyLexer lexer;
    sky_lexer_init(&lexer, "fn add(a int, b int) int { return a + b }", "test");

    SkyParser parser;
    sky_parser_init(&parser, &lexer);

    SkyASTNode *program = sky_parser_parse(&parser);

    if (!program) { FAIL("null AST"); return; }
    if (program->data.program.count < 1) { FAIL("no statements"); sky_ast_free(program); return; }

    SkyASTNode *fn = program->data.program.statements[0];
    if (fn->type != AST_FUNCTION) { FAIL("not a function"); sky_ast_free(program); return; }

    sky_ast_free(program);
    PASS();
}

static void test_if_statement(void) {
    TEST("Parse if/else statement");

    SkyLexer lexer;
    sky_lexer_init(&lexer,
        "if x > 10 { print(\"big\") } else { print(\"small\") }",
        "test");

    SkyParser parser;
    sky_parser_init(&parser, &lexer);

    SkyASTNode *program = sky_parser_parse(&parser);

    if (!program) { FAIL("null AST"); return; }
    if (program->data.program.count < 1) { FAIL("no statements"); sky_ast_free(program); return; }

    SkyASTNode *stmt = program->data.program.statements[0];
    if (stmt->type != AST_IF) { FAIL("not an if"); sky_ast_free(program); return; }

    sky_ast_free(program);
    PASS();
}

static void test_for_range(void) {
    TEST("Parse for..in range");

    SkyLexer lexer;
    sky_lexer_init(&lexer, "for i in 0..10 { print(i) }", "test");

    SkyParser parser;
    sky_parser_init(&parser, &lexer);

    SkyASTNode *program = sky_parser_parse(&parser);

    if (!program) { FAIL("null AST"); return; }
    if (program->data.program.count < 1) { FAIL("no statements"); sky_ast_free(program); return; }

    SkyASTNode *stmt = program->data.program.statements[0];
    if (stmt->type != AST_FOR && stmt->type != AST_FOR_IN) {
        FAIL("not a for");
        sky_ast_free(program);
        return;
    }

    sky_ast_free(program);
    PASS();
}

static void test_class_decl(void) {
    TEST("Parse class declaration");

    SkyLexer lexer;
    sky_lexer_init(&lexer,
        "class User { name string\n email string }",
        "test");

    SkyParser parser;
    sky_parser_init(&parser, &lexer);

    SkyASTNode *program = sky_parser_parse(&parser);

    if (!program) { FAIL("null AST"); return; }
    if (program->data.program.count < 1) { FAIL("no statements"); sky_ast_free(program); return; }

    SkyASTNode *cls = program->data.program.statements[0];
    if (cls->type != AST_CLASS) { FAIL("not a class"); sky_ast_free(program); return; }

    sky_ast_free(program);
    PASS();
}

int main(void) {
    printf("\n╔═══════════════════════════════════╗\n");
    printf("║     Sky Parser Test Suite         ║\n");
    printf("╚═══════════════════════════════════╝\n\n");

    test_let_statement();
    test_function_decl();
    test_if_statement();
    test_for_range();
    test_class_decl();

    printf("\n  Results: %d passed, %d failed\n\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}