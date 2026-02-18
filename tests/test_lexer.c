/* tests/test_lexer.c — Lexer test suite */
#include "../src/lexer.h"
#include "../src/token.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    printf("  TEST: %-40s ", name);

#define PASS() do { \
    printf("✓ PASS\n"); \
    tests_passed++; \
} while(0)

#define FAIL(msg) do { \
    printf("✗ FAIL: %s\n", msg); \
    tests_failed++; \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) { FAIL(msg); return; } \
} while(0)

/* ── Test: basic tokens ─────────────────────────────── */

static void test_basic_tokens(void) {
    TEST("Basic token types");

    SkyLexer lexer;
    sky_lexer_init(&lexer, "let x = 42", "test");

    SkyToken tok = sky_lexer_next(&lexer);
    ASSERT(tok.type == TOKEN_LET, "expected LET");

    tok = sky_lexer_next(&lexer);
    ASSERT(tok.type == TOKEN_IDENTIFIER, "expected IDENTIFIER");

    tok = sky_lexer_next(&lexer);
    ASSERT(tok.type == TOKEN_ASSIGN, "expected ASSIGN");

    tok = sky_lexer_next(&lexer);
    ASSERT(tok.type == TOKEN_INT_LITERAL, "expected INT_LITERAL");

    tok = sky_lexer_next(&lexer);
    ASSERT(tok.type == TOKEN_EOF, "expected EOF");

    PASS();
}

/* ── Test: string literals ──────────────────────────── */

static void test_strings(void) {
    TEST("String literals");

    SkyLexer lexer;
    sky_lexer_init(&lexer, "let s = \"hello world\"", "test");

    SkyToken tok = sky_lexer_next(&lexer); /* let */
    tok = sky_lexer_next(&lexer);          /* s */
    tok = sky_lexer_next(&lexer);          /* = */

    tok = sky_lexer_next(&lexer);
    ASSERT(tok.type == TOKEN_STRING_LITERAL, "expected STRING_LITERAL");
    ASSERT(strcmp(tok.value, "hello world") == 0, "wrong string value");

    PASS();
}

/* ── Test: keywords ─────────────────────────────────── */

static void test_keywords(void) {
    TEST("Keyword recognition");

    const char *keywords[] = {
        "fn", "class", "if", "else", "for", "while",
        "return", "import", "server", "route", "true", "false",
        NULL
    };

    SkyTokenType expected[] = {
        TOKEN_FN, TOKEN_CLASS, TOKEN_IF, TOKEN_ELSE,
        TOKEN_FOR, TOKEN_WHILE, TOKEN_RETURN, TOKEN_IMPORT,
        TOKEN_SERVER, TOKEN_ROUTE, TOKEN_TRUE, TOKEN_FALSE
    };

    for (int i = 0; keywords[i]; i++) {
        SkyLexer lexer;
        sky_lexer_init(&lexer, keywords[i], "test");
        SkyToken tok = sky_lexer_next(&lexer);
        if (tok.type != expected[i]) {
            char msg[128];
            snprintf(msg, sizeof(msg),
                     "keyword '%s' got type %d expected %d",
                     keywords[i], tok.type, expected[i]);
            FAIL(msg);
            return;
        }
    }

    PASS();
}

/* ── Test: operators ────────────────────────────────── */

static void test_operators(void) {
    TEST("Operator tokens");

    SkyLexer lexer;
    sky_lexer_init(&lexer, "+ - * / == != >= <= && ||", "test");

    SkyTokenType expected[] = {
        TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH,
        TOKEN_EQUAL_EQUAL, TOKEN_NOT_EQUAL,
        TOKEN_GREATER_EQUAL, TOKEN_LESS_EQUAL,
        TOKEN_AND, TOKEN_OR
    };

    for (int i = 0; i < 10; i++) {
        SkyToken tok = sky_lexer_next(&lexer);
        if (tok.type != expected[i]) {
            char msg[128];
            snprintf(msg, sizeof(msg),
                     "operator %d: got %d expected %d",
                     i, tok.type, expected[i]);
            FAIL(msg);
            return;
        }
    }

    PASS();
}

/* ── Test: float literal ────────────────────────────── */

static void test_floats(void) {
    TEST("Float literals");

    SkyLexer lexer;
    sky_lexer_init(&lexer, "3.14 0.5 100.0", "test");

    for (int i = 0; i < 3; i++) {
        SkyToken tok = sky_lexer_next(&lexer);
        ASSERT(tok.type == TOKEN_FLOAT_LITERAL, "expected FLOAT_LITERAL");
    }

    PASS();
}

/* ── Test: line numbers ─────────────────────────────── */

static void test_line_numbers(void) {
    TEST("Line number tracking");

    SkyLexer lexer;
    sky_lexer_init(&lexer, "let x = 1\nlet y = 2\nlet z = 3", "test");

    SkyToken tok = sky_lexer_next(&lexer); /* let */
    ASSERT(tok.line == 1, "line 1 expected");

    tok = sky_lexer_next(&lexer); /* x */
    tok = sky_lexer_next(&lexer); /* = */
    tok = sky_lexer_next(&lexer); /* 1 */
    tok = sky_lexer_next(&lexer); /* let (line 2) */
    ASSERT(tok.line == 2, "line 2 expected");

    tok = sky_lexer_next(&lexer); /* y */
    tok = sky_lexer_next(&lexer); /* = */
    tok = sky_lexer_next(&lexer); /* 2 */
    tok = sky_lexer_next(&lexer); /* let (line 3) */
    ASSERT(tok.line == 3, "line 3 expected");

    PASS();
}

/* ── Test: comments ─────────────────────────────────── */

static void test_comments(void) {
    TEST("Comment skipping");

    SkyLexer lexer;
    sky_lexer_init(&lexer, "let x = 1 // comment\nlet y = 2", "test");

    SkyToken tok;
    tok = sky_lexer_next(&lexer); /* let */
    tok = sky_lexer_next(&lexer); /* x */
    tok = sky_lexer_next(&lexer); /* = */
    tok = sky_lexer_next(&lexer); /* 1 */
    tok = sky_lexer_next(&lexer); /* let (line 2, comment skipped) */
    ASSERT(tok.type == TOKEN_LET, "expected LET after comment");
    ASSERT(tok.line == 2, "expected line 2");

    PASS();
}

/* ── Main ───────────────────────────────────────────── */

int main(void) {
    printf("\n╔═══════════════════════════════════╗\n");
    printf("║      Sky Lexer Test Suite         ║\n");
    printf("╚═══════════════════════════════════╝\n\n");

    test_basic_tokens();
    test_strings();
    test_keywords();
    test_operators();
    test_floats();
    test_line_numbers();
    test_comments();

    printf("\n  Results: %d passed, %d failed\n\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}