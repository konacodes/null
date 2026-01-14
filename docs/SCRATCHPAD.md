# Scratchpad - Shared Agent Memory

> **Last Updated:** 2026-01-14
> **Current Phase:** 2 - M7 COMPLETE (Self-Hosting) + CLI Support

---

## Current Milestone: M7 - Self-Hosting Complete + CLI

**Status:** ✓ COMPLETE - Full self-hosting with CLI argument support!

### Achievement Summary:
1. **nullc compiles itself** ✓ - mini.null with CLI arg support
2. **Self-compiled nullc compiles hello.null** ✓ - `./nullc2 examples/hello.null -o hello`
3. **Output runs correctly** ✓ - `./hello` prints "Hello, world!"
4. **All language tests pass** ✓ - 13/13 tests passing

### Key Files:
- `nullc/src/main.null` - Self-hosted compiler entry point (per prompt.md spec)
- `nullc/mini.null` - Self-hosted compiler source (2900+ lines) with CLI support
- `src/codegen.c` - C compiler with argc/argv support for main
- `std/map.null` - Hash map implementation (213 lines)
- `nullc_compiler` - C-compiled self-hosted compiler
- `nullc2` - Self-compiled self-hosted compiler (bootstrapped!)

### Final Fixes for M7:
1. **String escape lexer** - Handle backslash escapes in string scanning
2. **unescape_string()** - Convert `\"` → `"`, `\\` → `\`, `\n` → newline, etc.
3. **String literal parsing** - Apply unescape when extracting string values

---

## Previous Milestone: M4 - Parser in Null

**Status:** ✓ Complete - 12 tests passing

### Completed Features:
- Expression parsing (binary, unary, literals)
- Function declaration parsing
- If/while/for statement parsing
- Break/continue statements
- Variable declarations with type parsing
- Struct declaration parsing
- Enum declaration parsing
- Function call parsing
- Member access parsing
- Array index parsing
- Complete program parsing

### Tests Passing: 12/12
1. Expression: `1 + 2 * 3`
2. Function: `fn foo() -> i32 do ret 42 end`
3. If statement: `if true do ret 1 end`
4. Function call: `foo(1, 2)`
5. Member access: `obj.field`
6. Array index: `arr[0]`
7. Struct decl: `struct Foo do x :: i64 end`
8. Enum decl: `enum Color do RED GREEN end`
9. Break: `break`
10. For loop: `for let i :: i64 = 0; i < 10; i + 1 do break end`
11. Program: `fn main() -> i32 do ret 0 end`
12. Function with while: `fn test() -> i64 do let x :: i64 = 0 while x < 10 do break end ret x end`

### Key Files:
- `nullc/parser.null` - Self-hosted parser (1500+ lines)
- `nullc/lexer.null` - Self-hosted lexer (working)

---

## Codemap

### C Compiler (src/)

| File | Key Functions | Purpose |
|------|---------------|---------|
| `lexer.c` | `lexer_init()`, `lexer_next()`, `lexer_peek()` | Tokenization |
| `parser.c` | `parse()`, `parse_fn()`, `parse_expr()`, `parse_stmt()` | AST construction |
| `analyzer.c` | `analyze()`, `analyze_expr()`, `resolve_type()` | Type checking |
| `codegen.c` | `codegen_generate()`, `codegen_expr()`, `codegen_stmt()` | LLVM IR generation |
| `interp.c` | `interp_run()`, `interp_expr()`, `interp_stmt()` | Tree-walking interpreter |
| `main.c` | `main()`, `run_repl()`, `run_tests()` | CLI, orchestration |

### Key Structs

| Struct | File | Purpose |
|--------|------|---------|
| `Token` | `lexer.h` | Lexer output |
| `ASTNode` | `parser.h` | AST nodes (union-style) |
| `Type` | `parser.h` | Type representation |
| `Codegen` | `codegen.c` | LLVM codegen state |
| `Interp` | `interp.h` | Interpreter state |

### Type System (parser.h)

```c
typedef enum {
    TYPE_I8, TYPE_I16, TYPE_I32, TYPE_I64,
    TYPE_U8, TYPE_U16, TYPE_U32, TYPE_U64,
    TYPE_F32, TYPE_F64,
    TYPE_BOOL, TYPE_VOID, TYPE_PTR,
    TYPE_ARRAY, TYPE_SLICE, TYPE_STRUCT, TYPE_FUNCTION
} TypeKind;
```

---

## Current Focus

**Task:** M7 - Self-Hosting ✓ COMPLETE WITH CLI SUPPORT

**Status:** All Success Criteria Achieved!

**Bootstrap Chain:**
```bash
# Step 1: C compiler builds self-hosted compiler
./null build nullc/mini.null -o nullc_bin

# Step 2: Self-hosted compiler compiles itself
./nullc_bin nullc/mini.null -o nullc2

# Step 3: Self-compiled compiler compiles programs
./nullc2 examples/hello.null -o hello

# Step 4: Compiled program runs correctly
./hello  # Output: "Hello, world!"
```

**Verified:**
- Full bootstrap chain working with CLI arguments
- Self-compiled nullc2 produces working executables
- All 13 language tests pass
- Function return type handling fixed for pointer/i64/i32 returns

---

## Blockers

*None currently*

---

## Decisions Log

| Date | Decision | Rationale |
|------|----------|-----------|
| 2026-01-13 | Use LLVM sret for struct returns | Standard ABI convention, well-documented |
| 2026-01-13 | nullc will emit LLVM IR as text | Avoids linking LLVM from null, simpler FFI |
| 2026-01-13 | No generics initially | Use concrete types (MapStrI64, etc.) to ship faster |
| 2026-01-13 | Target ~15 sessions for self-host | Aggressive but achievable for focused AI work |

---

## Test Status

### Language Tests (tests/lang/)
```
arithmetic.null      PASS
functions.null       PASS
loops.null           PASS
variables.null       PASS
comparison.null      PASS
control_flow.null    PASS
structs.null         PASS
arrays.null          PASS
struct_return.null   PASS
enum_basic.null      PASS
enum_match.null      PASS
mem_test.null        PASS
string_test.null     PASS
--------------------------
Total: 13/13 PASS
```

---

## Session Log

### Session N+4 (2026-01-14) - CLI Support + Function Return Type Fix
**Full bootstrap chain with CLI arguments working!**

**Problem Identified:**
- Self-compiled nullc2 was segfaulting when trying to compile files
- Root cause: mini.null hardcoded ALL function calls to return i32
- This corrupted 64-bit pointers from functions like `get_argv`, `malloc`, `fopen`

**Fixes Applied:**
1. **C compiler argc/argv support** (`src/codegen.c`)
   - Main function now receives `(i32 %argc, i8** %argv)`
   - Global variables `@__nullc_argc` and `@__nullc_argv` store values
   - Helper functions `get_argc()` and `get_argv(i64)` for access

2. **Function return type handling** (`nullc/mini.null` lines ~1869-1950)
   - Detect pointer-returning functions: `call ptr @name(...) + ptrtoint`
   - Detect i64-returning functions: `call i64 @name(...)`
   - Keep i32 for others: `call i32 @name(...) + sext`

**Bootstrap Chain Verified:**
```bash
# C compiler builds mini.null → nullc_bin (53KB)
./null build nullc/mini.null -o nullc_bin

# nullc_bin self-compiles → nullc2 (53KB)
./nullc_bin nullc/mini.null -o nullc2

# nullc2 compiles hello.null → hello (16KB)
./nullc2 examples/hello.null -o hello

# hello runs correctly
./hello  # "Hello, world!"
```

**Success Criteria Met:**
1. ✅ `./nullc_bin nullc/mini.null -o nullc2` - nullc compiles itself
2. ✅ `./nullc2 examples/hello.null -o hello` - compiled compiler works
3. ✅ `./hello` prints "Hello, world!"
4. ✅ All 13 language tests pass

### Session N+3 (2026-01-14) - M7 SELF-HOSTING COMPLETE 🎉
**Major Achievement: null compiler compiles itself!**

**Problem Identified:**
- Self-compilation was truncating at 116 functions (missing main)
- Strings containing `\"` were being cut off early
- The `\"` in target triple and other strings caused lexer to terminate string early

**Fixes Applied:**
1. **String lexer escape handling** (lines 627-648 in mini.null)
   - When scanning strings, now skips character after backslash
   - Prevents `\"` from being seen as end of string

2. **unescape_string() function** (lines 405-444)
   - Converts escape sequences when extracting string values
   - `\"` → `"`, `\\` → `\`, `\n` → newline(10), `\t` → tab(9)

3. **String literal parsing** (line 1082)
   - Changed `lex_token_text(lex)` to `unescape_string(lex_token_text(lex))`

**Results:**
- Output grew from 7550 to 8215 lines, 116 to 121 functions
- `main` function now present
- Self-compiled binary produces identical output to Rust-compiled
- MD5 verification: both produce `2d76634223ce233392abdf03379a7539`

**The Bootstrap Chain:**
```
mini.null (source)
    ↓ [Rust compiler]
mini_v10 (binary)
    ↓ [compiles mini.null]
mini_out.ll (LLVM IR)
    ↓ [llc + gcc]
mini_self (self-compiled binary)
    ↓ [compiles hello.null]
hello_out.ll (identical to Rust-compiled output!)
```

### Session N+2 (2026-01-14) - End-to-End Compilation
**End-to-end pipeline working:**
- nullc/main.null created as compiler driver
- File I/O reads source files
- Parser handles newlines in real files (fixed TOK_NEWLINE handling in parse_statement)
- Codegen writes valid LLVM IR to file
- llc + clang produces working executables

**Key Fixes:**
- Added newline skipping to parse_statement and parse_declaration
- Added EOF and END handling in parse_statement to prevent errors
- Fixed parse_block_cont to handle null nodes from parse_statement
- Fixed parse_program to store both data and count of declarations

**Current State:**
- Codegen is still hardcoded (always emits `main returns 0`)
- AST walking attempted but crashed - needs more investigation
- Parse + codegen pipeline works for any valid null source

**What Works:**
```bash
./null run nullc/main.null  # Compiles /tmp/claude/test.null
llc -filetype=obj /tmp/claude/nullc_out.ll -o /tmp/claude/out.o
clang /tmp/claude/out.o -o /tmp/claude/out
./tmp/claude/out  # Returns 0
```

### Session N+1 (2026-01-14) - Major Progress
**M4 COMPLETE** - Self-hosted parser fully working
- Implemented struct/enum declaration parsing
- Implemented for/break/continue statement parsing
- Added skip_type() for proper type handling in var decls
- 12 parser tests passing
- Parser can parse complete programs with loops, vars, functions

**M5/M6 Foundations Created**
- nullc/analyzer.null (3 tests) - type checking infrastructure
- nullc/codegen.null (3 tests) - LLVM IR text emitter

**Total Tests:** 18 tests passing across lexer, parser, analyzer, codegen

### Session N (2026-01-13)
- Created Phase 2 spec (prompt.md)
- Created scratchpad
- Ready to begin struct returns implementation

---

## Quick Reference

### Build & Test
```bash
make                    # Build compiler
make test               # Run all tests
./null test tests/lang  # Run language tests only
./null repl             # Interactive REPL
```

### File Paths
```
src/codegen.c:XXX      # struct return implementation goes here
src/analyzer.c:XXX     # type checking for struct returns
tests/lang/structs.null # existing struct tests
```

---

## Notes

- The C compiler uses arena allocation (`arena.c`) for AST nodes
- Module system caches parsed modules to avoid re-parsing
- Error messages include source context (line + caret)
- Both JIT and interpreter must be updated for new features
