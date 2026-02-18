/* main.c — Sky compiler entry point */
#include "sky.h"
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "vm.h"
#include "debug.h"
#include "bytecode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* read_file(const char *path) {
    FILE *f;
    long size;
    char *buf;
    size_t read_count;
    f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = (char*)malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    read_count = fread(buf, 1, size, f);
    buf[read_count] = '\0';
    fclose(f);
    return buf;
}

static void run_file(const char *path) {
    char *source;
    SkyLexer lexer;
    SkyParser parser;
    SkyASTNode *ast;
    SkyChunk chunk;
    SkyCompiler compiler;
    SkyVM vm;
    SkyVMResult result;

    source = read_file(path);
    if (!source) return;

    sky_lexer_init(&lexer, source, path);
    sky_parser_init(&parser, &lexer);
    ast = sky_parser_parse(&parser);

    if (!ast) {
        fprintf(stderr, "Error: Failed to parse '%s'\n", path);
        free(source);
        return;
    }

    sky_chunk_init(&chunk);
    sky_compiler_init(&compiler, &chunk);

    if (!sky_compiler_compile(&compiler, ast)) {
        fprintf(stderr, "Error: Failed to compile '%s'\n", path);
        sky_ast_free(ast);
        sky_chunk_free(&chunk);
        free(source);
        return;
    }

    sky_chunk_write(&chunk, OP_HALT, 0);

    sky_vm_init(&vm);
    result = sky_vm_execute(&vm, &chunk);

    if (result != VM_OK) {
        fprintf(stderr, "Error: Runtime error in '%s'\n", path);
    }

    sky_vm_destroy(&vm);
    sky_ast_free(ast);
    sky_chunk_free(&chunk);
    free(source);
}

static void check_file(const char *path) {
    char *source;
    SkyLexer lexer;
    SkyParser parser;
    SkyASTNode *ast;

    source = read_file(path);
    if (!source) return;

    sky_lexer_init(&lexer, source, path);
    sky_parser_init(&parser, &lexer);
    ast = sky_parser_parse(&parser);

    if (ast) {
        printf("OK: %s\n", path);
        sky_ast_free(ast);
    } else {
        printf("FAIL: %s\n", path);
    }
    free(source);
}

static void print_usage(void) {
    printf("Sky Programming Language v%s\n\n", SKY_VERSION_STRING);
    printf("Usage:\n");
    printf("  sky run <file.sky>    Compile and run\n");
    printf("  sky check <file.sky>  Syntax check\n");
    printf("  sky version           Show version\n");
    printf("  sky help              Show this help\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return 0;
    }

    if (strcmp(argv[1], "version") == 0) {
        printf("Sky v%s\n", SKY_VERSION_STRING);
        return 0;
    }

    if (strcmp(argv[1], "help") == 0) {
        print_usage();
        return 0;
    }

    if (strcmp(argv[1], "run") == 0 || strcmp(argv[1], "serve") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: No file specified\n");
            return 1;
        }
        run_file(argv[2]);
        return 0;
    }

    if (strcmp(argv[1], "check") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: No file specified\n");
            return 1;
        }
        check_file(argv[2]);
        return 0;
    }

    /* Try to run as file directly */
    run_file(argv[1]);
    return 0;
}
