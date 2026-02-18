# Makefile for Sky Programming Language

CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -O2 -std=c11 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lpthread -lm

# Source files
SRC_CORE = src/main.c      \
           src/lexer.c      \
           src/parser.c     \
           src/ast.c        \
           src/analyzer.c   \
           src/compiler.c   \
           src/vm.c         \
           src/value.c      \
           src/table.c      \
           src/memory.c     \
           src/debug.c      \
           src/module.c

SRC_RUNTIME = src/runtime/http_server.c \
              src/runtime/security.c    \
              src/runtime/db.c          \
              src/runtime/jwt.c         \
              src/runtime/crypto.c      \
              src/runtime/async.c

SRC = $(SRC_CORE) $(SRC_RUNTIME)
OBJ = $(SRC:.c=.o)

# Output binary
TARGET = sky

# Default target
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "  ✓ Sky compiler built successfully"
	@echo "  Run: ./sky run <file.sky>"
	@echo ""

# Compile .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build
debug: CFLAGS = -Wall -Wextra -g -O0 -std=c11 -D_POSIX_C_SOURCE=200809L -DSKY_DEBUG
debug: clean $(TARGET)

# Tests
test: $(TARGET)
	@echo "Running tests..."
	@./sky check tests/samples/hello.sky 2>/dev/null && echo "  ✓ hello.sky" || echo "  ✗ hello.sky"
	@./sky check tests/samples/api.sky 2>/dev/null && echo "  ✓ api.sky" || echo "  ✗ api.sky"
	@echo "Done."

# Clean
clean:
	rm -f $(OBJ) $(TARGET)
	@echo "  Cleaned."

# Install
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	@echo "  Installed to /usr/local/bin/sky"

# Uninstall
uninstall:
	rm -f /usr/local/bin/sky
	@echo "  Uninstalled."

# Dependencies (header tracking)
src/main.o: src/main.c src/lexer.h src/parser.h src/vm.h
src/lexer.o: src/lexer.c src/lexer.h src/token.h src/memory.h
src/parser.o: src/parser.c src/parser.h src/ast.h src/token.h
src/ast.o: src/ast.c src/ast.h src/memory.h
src/analyzer.o: src/analyzer.c src/analyzer.h src/ast.h
src/compiler.o: src/compiler.c src/compiler.h src/ast.h src/bytecode.h
src/vm.o: src/vm.c src/vm.h src/bytecode.h src/value.h src/table.h
src/value.o: src/value.c src/value.h src/memory.h
src/table.o: src/table.c src/table.h src/value.h src/memory.h
src/memory.o: src/memory.c src/memory.h
src/debug.o: src/debug.c src/debug.h src/bytecode.h src/value.h
src/module.o: src/module.c src/module.h src/value.h

.PHONY: all debug test clean install uninstall