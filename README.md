# null

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Built with Claude](https://img.shields.io/badge/Built%20with-Claude-blueviolet)](https://claude.ai)

A compiled programming language with LLVM backend, built entirely by Claude.

## Features

- **Dual execution model** - JIT compilation via LLVM or tree-walking interpreter
- **Interactive REPL** - Explore the language interactively
- **Native compilation** via LLVM - compiles directly to machine code
- **Clean syntax** with `do`/`end` blocks, `::` type annotations, `@` directives
- **Strong typing** with type inference
- **Structs** with field access
- **Arrays** with indexing
- **Control flow**: if/elif/else, while loops, for..in ranges with break/continue
- **Functions** with parameters and return values
- **Constants** with `const` keyword
- **Module system** with `@use` imports and cycle detection
- **Enhanced error messages** with source context and hints

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

# Run tests in a directory
./null test tests/lang
```

## Syntax Overview

### Hello World
```null
@use "std/io"

fn main() -> i32 do
    io_print("Hello, world!")
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

let p = Point { x = 10, y = 20 }
let sum :: i64 = p.x + p.y
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
| `[T]` | Slice |

## Project Structure

```
null/
├── src/
│   ├── main.c          # Entry point, CLI
│   ├── lexer.c/.h      # Tokenization
│   ├── parser.c/.h     # AST construction
│   ├── analyzer.c/.h   # Semantic analysis
│   ├── codegen.c/.h    # LLVM code generation
│   ├── interp.c/.h     # Tree-walking interpreter
│   └── arena.c/.h      # Arena memory allocator
├── std/
│   ├── io.null         # I/O operations
│   └── math.null       # Math functions
├── tests/lang/         # Language tests
├── examples/           # Example programs
└── docs/
    ├── GRAMMAR.md      # Formal EBNF grammar
    ├── SYNTAX.md       # Syntax reference
    └── MODULES.md      # Module system docs
```

## Building

Requirements:
- LLVM 21+ development libraries
- Clang compiler

```bash
make        # Build compiler
make test   # Run tests
make clean  # Clean build
```

## Running Tests

```bash
# Run all tests (compiler + language)
make test

# Run language tests only
./null test tests/lang

# Run a specific test (JIT)
./null tests/lang/arithmetic.null

# Run a specific test (interpreter)
./null interp tests/lang/arithmetic.null
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

## Known Limitations

- Struct returns from functions are not yet supported
- No garbage collection (manual memory management)
- No generics or templates
- No async/concurrency

## License

MIT
