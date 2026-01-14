# Scratchpad - Shared Agent Memory

> **Last Updated:** 2026-01-13
> **Current Phase:** 2.1 (Language Features)

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

**Task:** Implement struct returns from functions

**Status:** Not started

**Approach:**
1. Research LLVM sret convention
2. Modify `codegen_function()` to detect struct return types
3. Add hidden sret parameter for struct-returning functions
4. Update call sites to pass destination pointer
5. Write test case for struct returns
6. Verify with both JIT and AOT

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
