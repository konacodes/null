# Changelog

All notable changes to the null programming language will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- Enhanced error messages with source context and hints
- REPL (Read-Eval-Print Loop) with `:help`, `:exit`, `:clear` commands
- Complete standard library I/O (`print_u64`, `print_bool`, `print_int_ln`)
- `break` and `continue` statements for loops
- `const` keyword for compile-time constants
- VS Code syntax highlighting extension
- Tutorial documentation (`docs/TUTORIAL.md`)
- Community infrastructure (CONTRIBUTING.md, CODE_OF_CONDUCT.md)
- Arena allocator for efficient memory management
- Tree-walking interpreter mode (`./null interp`)
- Formal EBNF grammar specification (`docs/GRAMMAR.md`)

### Fixed
- Buffer overflow in multi-line comment parsing
- Command injection vulnerability (now uses fork/execlp)
- Use-after-free in scope management
- Integer overflow in array size handling
- Short-circuit evaluation for `and`/`or` operators
- Struct field ordering in initializers
- String escape sequences (`\n`, `\t`, `\r`, `\\`, `\"`, `\0`)
- Binary operation type checking

### Changed
- Dynamic arrays now use geometric growth (O(1) amortized insertion)
- Module system with cycle detection and caching
- Test runner reports pass/fail counts
- Hardcoded paths removed (uses executable-relative paths)

## [0.1.0] - 2026-01-13

### Added
- Initial release
- LLVM-based compiler with JIT execution
- Lexer, parser, semantic analyzer, code generator
- Basic types: i8-i64, u8-u64, f32, f64, bool, void, ptr
- Variables with `let` and `mut`
- Functions with parameters and return values
- Structs with field access
- Fixed-size arrays
- Control flow: if/elif/else, while, for..in
- Module system with `@use`
- FFI with `@extern "C"`
- Standard library: std/io.null, std/math.null
