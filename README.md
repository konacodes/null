# [\\]null

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/jace/null/releases/tag/v1.0.0)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Built with Claude](https://img.shields.io/badge/Built%20with-Claude-blueviolet)](https://claude.ai)
[![Self-Hosted](https://img.shields.io/badge/Self--Hosted-Yes-green.svg)](#self-hosting)

A compiled programming language with LLVM backend, built entirely by Claude. **null is self-hosted** - the compiler can compile itself.

## Highlights

- **Self-hosted compiler** - null compiles itself (see [Self-Hosting](#self-hosting))
- **Dual execution model** - JIT compilation via LLVM or tree-walking interpreter
- **Interactive REPL** - Explore the language interactively
- **Native compilation** - Compiles directly to machine code via LLVM
- **Clean syntax** - `do`/`end` blocks, `::` type annotations, `@` directives
- **Strong typing** with type inference
- **Enums and Structs** with field access and struct returns
- **Module system** with `@use` imports and cycle detection
- **Standard library** - mem, string, array, map, file, io, math

## Quick Start

```bash
# Build the compiler
make

# Run a program (JIT compiled)
./null examples/hello.null

# Run with interpreter (no LLVM required at runtime)
./null interp examples/hello.null

# Compile to standalone executable
./null build program.null -o program

# Run tests
./null test tests/lang
```

## Self-Hosting

null achieved self-hosting - the compiler can compile itself:

```bash
# Build the self-hosted compiler from source
./null build nullc/src/main.null -o nullc

# The self-hosted compiler can compile itself
./nullc nullc/src/main.null -o nullc2

# And produce working executables
./nullc2 examples/hello.null -o hello
./hello  # "Hello, world!"
```

The self-hosted compiler (`nullc/src/main.null`) is ~2,700 lines of null code containing:
- Lexer (tokenization)
- Parser (AST construction)
- Codegen (LLVM IR text output)
- CLI argument handling

## Syntax Overview

### Hello World
```null
@extern "C" do
    fn puts(s :: ptr<u8>) -> i64
end

fn main() -> i32 do
    puts("Hello, world!")
    ret 0
end
```

### Variables
```null
let x :: i64 = 42       -- immutable
mut y :: i64 = 10       -- mutable
let z = 100             -- type inference
```

### Functions
```null
fn add(a :: i64, b :: i64) -> i64 do
    ret a + b
end
```

### Structs
```null
struct Point do
    x :: i64
    y :: i64
end

fn make_point(x :: i64, y :: i64) -> Point do
    ret Point { x = x, y = y }
end

let p :: Point = make_point(10, 20)
```

### Enums
```null
enum Color do
    Red
    Green
    Blue
end

let c :: Color = Color::Red
```

### Control Flow
```null
if x > 0 do
    io_print("positive")
elif x < 0 do
    io_print("negative")
else do
    io_print("zero")
end

while i < 10 do
    i = i + 1
end

for i in 0..10 do
    -- loop body
end
```

### Arrays
```null
let arr :: [i64; 5] = [1, 2, 3, 4, 5]
let first :: i64 = arr[0]
```

## Types

| Type | Description |
|------|-------------|
| `i8`, `i16`, `i32`, `i64` | Signed integers |
| `u8`, `u16`, `u32`, `u64` | Unsigned integers |
| `f32`, `f64` | Floating point |
| `bool` | Boolean |
| `void` | No value |
| `ptr<T>` | Pointer |
| `[T; N]` | Fixed array |

## Standard Library

| Module | Description |
|--------|-------------|
| `std/mem.null` | Memory allocation (malloc, free, memcpy) |
| `std/string.null` | String operations (str_eq, str_len, str_dup) |
| `std/array.null` | Dynamic arrays |
| `std/map.null` | Hash map with string keys |
| `std/file.null` | File I/O (read, write, seek) |
| `std/io.null` | Console I/O (print functions) |
| `std/math.null` | Math operations |

## Project Structure

```
null/
├── src/                    # C compiler source
│   ├── main.c              # Entry point, CLI
│   ├── lexer.c/.h          # Tokenization
│   ├── parser.c/.h         # AST construction
│   ├── analyzer.c/.h       # Semantic analysis
│   ├── codegen.c/.h        # LLVM code generation
│   ├── interp.c/.h         # Tree-walking interpreter
│   └── arena.c/.h          # Arena memory allocator
├── nullc/                  # Self-hosted compiler
│   ├── src/                # Compiler modules
│   │   ├── main.null       # CLI, orchestration
│   │   ├── lexer.null      # Tokenization
│   │   ├── parser.null     # AST construction
│   │   ├── analyzer.null   # Type checking
│   │   ├── codegen.null    # LLVM IR generation
│   │   └── types.null      # Type definitions
│   └── tests/              # Compiler tests
├── std/                    # Standard library
├── tests/lang/             # Language tests
├── examples/               # Example programs
└── docs/                   # Documentation
```

## Building

Requirements:
- LLVM 14+ development libraries
- Clang compiler

```bash
make        # Build compiler
make test   # Run tests
make clean  # Clean build
```

## Running Tests

```bash
# Run all language tests
./null test tests/lang

# Run self-hosted compiler tests
./null run nullc/tests/lexer_test.null
./null run nullc/tests/parser_test.null
./null run nullc/tests/analyzer_test.null
./null run nullc/tests/codegen_test.null
```

## Examples

- `examples/hello.null` - Hello World
- `examples/fibonacci.null` - Fibonacci sequence
- `examples/vector_math.null` - Vector operations with structs

## Design Philosophy

null follows a **minimal kernel** design:
- The compiler core contains only primitives
- Everything else lives in the standard library
- Simple, consistent syntax that's easy to parse
- Self-hosting proves the language is powerful enough for real systems work

## License

MIT
