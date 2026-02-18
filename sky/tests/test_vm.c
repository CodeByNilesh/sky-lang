/* tests/test_vm.c — VM test suite */
#include "../src/vm.h"
#include "../src/debug.h"
#include "../src/value.h"
#include "../src/bytecode.h"
#include <stdio.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  TEST: %-40s ", name);
#define PASS() do { printf("✓ PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("✗ FAIL: %s\n", msg); tests_failed++; } while(0)

static void test_arithmetic(void) {
    TEST("VM arithmetic (2 + 3)");

    SkyChunk chunk;
    sky_chunk_init(&chunk, "test_add");

    /* Push 2 */
    int c1 = sky_chunk_add_constant(&chunk, SKY_INT(2));
    sky_chunk_write(&chunk, OP_CONSTANT, 1);
    sky_chunk_write(&chunk, (uint8_t)c1, 1);

    /* Push 3 */
    int c2 = sky_chunk_add_constant(&chunk, SKY_INT(3));
    sky_chunk_write(&chunk, OP_CONSTANT, 1);
    sky_chunk_write(&chunk, (uint8_t)c2, 1);

    /* Add */
    sky_chunk_write(&chunk, OP_ADD, 1);

    /* Print */
    sky_chunk_write(&chunk, OP_PRINT, 1);

    /* Halt */
    sky_chunk_write(&chunk, OP_HALT, 1);

    SkyVM vm;
    sky_vm_init(&vm);

    SkyVMResult result = sky_vm_execute(&vm, &chunk);

    if (result != VM_OK) {
        FAIL("VM execution failed");
    } else {
        PASS();
    }

    sky_vm_destroy(&vm);
    sky_chunk_free(&chunk);
}

static void test_string_concat(void) {
    TEST("VM string concat");

    SkyChunk chunk;
    sky_chunk_init(&chunk, "test_concat");

    int c1 = sky_chunk_add_constant(&chunk, SKY_STRING("Hello "));
    sky_chunk_write(&chunk, OP_CONSTANT, 1);
    sky_chunk_write(&chunk, (uint8_t)c1, 1);

    int c2 = sky_chunk_add_constant(&chunk, SKY_STRING("Sky!"));
    sky_chunk_write(&chunk, OP_CONSTANT, 1);
    sky_chunk_write(&chunk, (uint8_t)c2, 1);

    sky_chunk_write(&chunk, OP_ADD, 1);
    sky_chunk_write(&chunk, OP_PRINT, 1);
    sky_chunk_write(&chunk, OP_HALT, 1);

    SkyVM vm;
    sky_vm_init(&vm);

    SkyVMResult result = sky_vm_execute(&vm, &chunk);

    if (result != VM_OK) {
        FAIL("VM execution failed");
    } else {
        PASS();
    }

    sky_vm_destroy(&vm);
    sky_chunk_free(&chunk);
}

static void test_comparison(void) {
    TEST("VM comparison (5 > 3)");

    SkyChunk chunk;
    sky_chunk_init(&chunk, "test_cmp");

    int c1 = sky_chunk_add_constant(&chunk, SKY_INT(5));
    sky_chunk_write(&chunk, OP_CONSTANT, 1);
    sky_chunk_write(&chunk, (uint8_t)c1, 1);

    int c2 = sky_chunk_add_constant(&chunk, SKY_INT(3));
    sky_chunk_write(&chunk, OP_CONSTANT, 1);
    sky_chunk_write(&chunk, (uint8_t)c2, 1);

    sky_chunk_write(&chunk, OP_GREATER, 1);
    sky_chunk_write(&chunk, OP_PRINT, 1);
    sky_chunk_write(&chunk, OP_HALT, 1);

    SkyVM vm;
    sky_vm_init(&vm);

    SkyVMResult result = sky_vm_execute(&vm, &chunk);
    if (result != VM_OK) { FAIL("VM execution failed"); }
    else { PASS(); }

    sky_vm_destroy(&vm);
    sky_chunk_free(&chunk);
}

static void test_boolean_ops(void) {
    TEST("VM boolean operations");

    SkyChunk chunk;
    sky_chunk_init(&chunk, "test_bool");

    sky_chunk_write(&chunk, OP_TRUE, 1);
    sky_chunk_write(&chunk, OP_FALSE, 1);
    sky_chunk_write(&chunk, OP_OR, 1);
    sky_chunk_write(&chunk, OP_PRINT, 1);
    sky_chunk_write(&chunk, OP_HALT, 1);

    SkyVM vm;
    sky_vm_init(&vm);

    SkyVMResult result = sky_vm_execute(&vm, &chunk);
    if (result != VM_OK) { FAIL("VM execution failed"); }
    else { PASS(); }

    sky_vm_destroy(&vm);
    sky_chunk_free(&chunk);
}

int main(void) {
    printf("\n╔═══════════════════════════════════╗\n");
    printf("║       Sky VM Test Suite           ║\n");
    printf("╚═══════════════════════════════════╝\n\n");

    test_arithmetic();
    test_string_concat();
    test_comparison();
    test_boolean_ops();

    printf("\n  Results: %d passed, %d failed\n\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}