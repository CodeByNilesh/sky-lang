# Sky Programming Language

A compiled programming language designed for fast, secure API development.

C++ performance. Python simplicity. Security built-in.

## Example

    server api on 3000 {
        route GET "/hello" {
            respond 200 json({"msg": "Hello from Sky!"})
        }
    }

## Features

- Fast. Compiles to bytecode, runs on a custom VM.
- Secure. Built-in attack detection and prevention.
- API-First. Native server, route, respond keywords.
- Simple. Clean syntax, type inference, zero boilerplate.
- Async. Non-blocking I/O with async and await.

## Install

    git clone https://github.com/yourname/sky.git
    cd sky
    make
    sudo make install

## Usage

    sky run app.sky
    sky build app.sky -o app
    sky serve api.sky
    sky repl
    sky check app.sky
    sky version

## Variables

    let name = "Sky"
    let age = 25
    let items = [1, 2, 3]

## Functions

    fn add(a int, b int) int {
        return a + b
    }

## Classes

    class User {
        name string
        email string

        fn display(self) {
            print(self.name)
        }
    }

## Security

    security {
        rate_limit 100/min per ip
        on sql_injection {
            block request
            blacklist ip for 72h
        }
    }

## Architecture

    source.sky -> Lexer -> Parser -> AST -> Analyzer -> Compiler -> VM

## Build from Source

    make
    make debug
    make test
    make clean

## Adding New Features

To add any new keyword or feature:

1. Add token type in src/token.h
2. Add lexer rule in src/lexer.c
3. Add AST node type in src/ast.h
4. Add parser rule in src/parser.c
5. Add compiler rule in src/compiler.c
6. Add VM instruction in src/vm.c

6 files, same pattern every time.

## Error Messages

    Error in app.sky at line 12:
        let x = "hello" + 5
                        ^
        Cannot add string and int.
        Hint: use str(5) to convert.

## License

MIT License
