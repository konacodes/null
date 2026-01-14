# Scratchpad - Shared Agent Memory

> **Last Updated:** 2026-01-14
> **Current Phase:** 2 - M4 (Self-Hosted Parser)

---

## Current Milestone: M4 - Parser in Null

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

**Task:** M5 - Self-Hosted Analyzer

**Status:** M4 Complete, Ready for M5

**Next Steps:**
1. Implement type checking in null
2. Add symbol table management
3. Implement semantic analysis

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
arithmetic.null    PASS
functions.null     PASS
loops.null         PASS
variables.null     PASS
comparison.null    PASS
control_flow.null  PASS
structs.null       PASS
arrays.null        PASS
------------------------
Total: 8/8 PASS
```

### Feature Tests (pending)
```
struct_return.null    NOT WRITTEN
enum_basic.null       NOT WRITTEN
dynamic_array.null    NOT WRITTEN
```

---

## Session Log

### Session N+1 (2026-01-14)
- **M4 COMPLETE** - Self-hosted parser fully working
- Implemented struct/enum declaration parsing
- Implemented for/break/continue statement parsing
- Added skip_type() for proper type handling in var decls
- 12 tests passing
- Parser can parse complete programs with loops, vars, functions

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
