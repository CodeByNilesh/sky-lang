# Sky Language — Type System

## Primitive Types

| Type    | Example          | Description            |
|---------|------------------|------------------------|
| int     | 42               | 64-bit signed integer  |
| float   | 3.14             | 64-bit double float    |
| string  | "hello"          | UTF-8 string           |
| bool    | true / false     | Boolean                |
| nil     | nil              | Null / absence value   |

## Composite Types

| Type    | Example               | Description            |
|---------|-----------------------|------------------------|
| array   | [1, 2, 3]             | Dynamic array          |
| map     | {"key": "value"}      | Hash map               |

## Type Inference

Sky uses type inference for let statements:

    let x = 42        // x is int
    let s = "hello"   // s is string
    let f = 3.14      // f is float
    let ok = true     // ok is bool
    let items = [1,2] // items is array

## Explicit Types in Functions

Functions declare parameter and return types explicitly:

    fn add(a int, b int) int {
        return a + b
    }

    fn concat(a string, b string) string {
        return a + b
    }

    fn is_valid(x int) bool {
        return x > 0
    }

No return type means void:

    fn log(msg string) {
        print(msg)
    }

## Class Types

Classes create custom named types:

    class User {
        name string
        age int
    }

    class Product {
        title string
        price float
        in_stock bool
    }

## Type Conversion

    let x = 42
    let s = str(x)       // "42"
    let f = float(x)     // 42.0
    let i = int(3.14)    // 3
    let b = bool(1)      // true

## Array Types

    let nums = [1, 2, 3]
    let names = ["Alice", "Bob"]
    let mixed = [1, "hello", true]

## Map Types

    let config = {
        "host": "localhost",
        "port": "8080",
        "debug": "true"
    }

## Nil Safety

Sky tracks nil values to prevent null pointer errors:

    let user = db.find("users", email)

    if user == nil {
        respond 404 json({"error": "not found"})
    }

    print(user.name)

## Type Checking Rules

1. Arithmetic operators require matching numeric types
2. String + performs concatenation
3. Comparison operators work on same types
4. Boolean operators require bool operands
5. Array indexing returns element type
6. Map access returns value type or nil
