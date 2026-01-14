# Changelog

All notable changes to the null programming language will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

Nothing yet.

## [1.0.0] - 2026-01-14

**null is now self-hosted!** The compiler can compile itself.

### Added
- **Self-hosted compiler** (`nullc/src/main.null`) - ~2,700 lines of null code
  - Full lexer with tokenization
  - Recursive descent parser with AST construction
  - LLVM IR text generation
  - CLI argument handling (argc/argv support)
- Self-hosted compiler modules:
  - `nullc/src/types.null` - Token, AST node, and type definitions
  - `nullc/src/lexer.null` - Tokenization
  - `nullc/src/parser.null` - AST construction
  - `nullc/src/analyzer.null` - Type checking
  - `nullc/src/codegen.null` - LLVM IR generation
- Component test suite:
  - `nullc/tests/lexer_test.null` - Lexer operation tests
  - `nullc/tests/parser_test.null` - Parser operation tests
  - `nullc/tests/analyzer_test.null` - Type system tests
  - `nullc/tests/codegen_test.null` - Code generation tests
- Standard library additions:
  - `std/map.null` - Hash map with string keys (djb2 hash, separate chaining)
  - `std/array.null` - Dynamic arrays
  - `std/file.null` - File I/O operations
  - `std/mem.null` - Memory allocation (malloc, free, memcpy)
  - `std/string.null` - String operations (str_eq, str_len, str_dup)
- Struct return values from functions
- Enum types with `::` variant access
- Native compilation to standalone executables (`./null build`)

### Changed
- Main function now supports argc/argv parameters
- Function return type detection for ptr vs i64 vs i32

### Technical
- Full bootstrap chain verified: C compiler → nullc_bin → nullc2 → executables
- 33 tests passing (13 language + 20 component tests)
- LLVM IR text output (no LLVM C API dependency in self-hosted compiler)

## [0.1.0] - 2026-01-13

Initial public release of the null programming language.

### Added
- LLVM-based compiler with JIT execution
- Tree-walking interpreter mode (`./null interp`)
- Interactive REPL with `:help`, `:exit`, `:clear`, `:env` commands
- Lexer, parser, semantic analyzer, code generator
- Arena allocator for efficient memory management
- Basic types: i8-i64, u8-u64, f32, f64, bool, void, ptr
- Variables with `let` (immutable) and `mut` (mutable)
- Constants with `const` keyword
- Functions with parameters and return values
- Structs with field access
- Fixed-size arrays with indexing
- Control flow: if/elif/else, while, for..in ranges
- Loop control: `break` and `continue` statements
- Module system with `@use` and cycle detection
- FFI with `@extern "C"`
- Standard library: std/io.null, std/math.null
- Complete I/O functions: `print_u64`, `print_bool`, `print_int_ln`, `io_print`
- Enhanced error messages with source context and hints
- VS Code syntax highlighting extension
- Formal EBNF grammar specification (`docs/GRAMMAR.md`)
- Tutorial documentation (`docs/TUTORIAL.md`)
- Community infrastructure (CONTRIBUTING.md, CODE_OF_CONDUCT.md, LICENSE)

### Security
- Safe command execution using fork/execlp (no shell injection)
- Bounds checking on array operations
- Integer overflow protection in array size handling

### Technical
- Short-circuit evaluation for `and`/`or` operators
- Geometric growth for dynamic arrays (O(1) amortized insertion)
- Module caching to prevent duplicate compilation
- String escape sequences (`\n`, `\t`, `\r`, `\\`, `\"`, `\0`)
