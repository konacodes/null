# null - Phase 2: Self-Hosting Specification

## Mission

Bootstrap null by writing a null compiler in null itself. This is an AI research project exploring how far autonomous AI agents can push systems programming. The goal is a fully self-hosted compiler within ~15 focused sessions.

## Philosophy

The original null compiler (written in C) is complete and working. Now we write `nullc` - a null compiler written in null. This proves the language is powerful enough for real systems work and creates a beautiful recursive loop: null compiling null.

**Key insight:** AI can work faster than humans. When stuck, research - don't guess. Use web search, read LLVM docs, study other compilers. This is engineering, not a test.

## Architecture: Subagents with Shared Scratchpad

This project uses multiple AI agents working in parallel. All agents share a scratchpad file for coordination:

```
docs/SCRATCHPAD.md   # Shared state between all agents
```

### Scratchpad Contents

```markdown
# Scratchpad - Shared Agent Memory

## Codemap
[Auto-updated map of key functions/structs and their locations]

## Current Focus
[What's being worked on right now]

## Blockers
[Issues that need resolution]

## Decisions Log
[Architectural decisions made and why]

## Test Status
[Which tests pass/fail]
```

### Agent Roles

| Agent | Responsibility |
|-------|----------------|
| **Architect** | Design decisions, breaks down tasks, updates scratchpad |
| **Implementer** | Writes code, follows architect's plan |
| **Tester** | Runs tests, reports failures, writes new tests |
| **Researcher** | Web search, reads docs, solves blockers |

Agents read scratchpad before working, write updates after. This prevents duplicate work and maintains shared understanding.

---

## Phase 2.1: Language Features (Prerequisites)

Before self-hosting, null needs these features in the C compiler:

### P0: Critical (Blockers)

#### Struct Returns from Functions
```null
fn create_token(kind :: TokenKind, line :: i64) -> Token do
    ret Token { kind = kind, line = line, col = 0 }
end
```
**Implementation:** Update `codegen.c` to use LLVM's `sret` (struct return) convention for functions returning structs. The struct is passed as a hidden first parameter pointer.

**Files:** `src/codegen.c`, `src/analyzer.c`

#### Enums
```null
enum TokenKind do
    EOF
    IDENT
    NUMBER
    STRING
    PLUS
    MINUS
    -- etc
end

let tok :: TokenKind = TokenKind.IDENT
```
**Implementation:** Integer-backed enums. Each variant is an i32. Support `EnumName.Variant` syntax.

**Files:** `src/lexer.c`, `src/parser.c`, `src/analyzer.c`, `src/codegen.c`, `src/interp.c`

### P1: High Priority

#### Dynamic Arrays (Slices)
```null
mut tokens :: [Token] = []
tokens = append(tokens, new_token)
let len :: i64 = length(tokens)
```
**Implementation:** Fat pointer (ptr + length + capacity). `append` reallocates via malloc when needed.

#### String Type
```null
let s :: string = "hello"
let c :: u8 = s[0]
let sub :: string = slice(s, 0, 3)
if s == "hello" do ... end
```
**Implementation:** `string` is `ptr<u8>` + length. No null terminator required internally.

#### Methods on Types
```null
fn (t :: Token) is_keyword() -> bool do
    ret t.kind >= TokenKind.FN and t.kind <= TokenKind.END
end

-- called as:
if tok.is_keyword() do ... end
```

### P2: Nice to Have

#### Match Expression
```null
let name :: string = match tok.kind do
    TokenKind.PLUS -> "plus"
    TokenKind.MINUS -> "minus"
    _ -> "unknown"
end
```

#### Result Type (Error Handling)
```null
fn parse_expr() -> Result<Expr, Error> do
    if error_condition do
        ret Err(Error { msg = "unexpected token" })
    end
    ret Ok(expr)
end
```

---

## Phase 2.2: Standard Library Expansion

### std/mem.null
```null
@extern "C" do
    fn malloc(size :: u64) -> ptr<u8>
    fn realloc(p :: ptr<u8>, size :: u64) -> ptr<u8>
    fn free(p :: ptr<u8>) -> void
    fn memcpy(dst :: ptr<u8>, src :: ptr<u8>, n :: u64) -> ptr<u8>
end

fn alloc<T>(count :: u64) -> ptr<T> do
    ret malloc(count * sizeof(T))
end
```

### std/string.null
```null
fn str_eq(a :: string, b :: string) -> bool
fn str_concat(a :: string, b :: string) -> string
fn str_slice(s :: string, start :: i64, end :: i64) -> string
fn str_find(haystack :: string, needle :: string) -> i64
fn str_from_cstr(s :: ptr<u8>) -> string
fn str_to_cstr(s :: string) -> ptr<u8>
```

### std/array.null
```null
fn array_new<T>() -> [T]
fn array_push<T>(arr :: ptr<[T]>, val :: T) -> void
fn array_pop<T>(arr :: ptr<[T]>) -> T
fn array_get<T>(arr :: [T], idx :: i64) -> T
fn array_len<T>(arr :: [T]) -> i64
```

### std/map.null
```null
struct Map<K, V> do
    buckets :: [ptr<Entry<K, V>>]
    size :: i64
end

fn map_new<K, V>() -> Map<K, V>
fn map_set<K, V>(m :: ptr<Map<K, V>>, key :: K, val :: V) -> void
fn map_get<K, V>(m :: Map<K, V>, key :: K) -> Option<V>
fn map_has<K, V>(m :: Map<K, V>, key :: K) -> bool
```

*Note: Generics may not be implemented. Use concrete types initially: `MapStrI64`, `MapStrPtr`, etc.*

---

## Phase 2.3: The Self-Hosted Compiler (nullc)

### Directory Structure
```
nullc/
  src/
    main.null       # CLI, orchestration
    lexer.null      # Tokenization
    parser.null     # AST construction
    analyzer.null   # Type checking, semantic analysis
    codegen.null    # LLVM IR text output
    types.null      # Token, AST, Type definitions
  tests/
    lexer_test.null
    parser_test.null
    ...
```

### Compilation Strategy

nullc outputs LLVM IR as text files, then shells out to `llc` and `clang` for machine code:

```null
fn compile(source :: string, output :: string) -> i32 do
    let tokens :: [Token] = lex(source)
    let ast :: Module = parse(tokens)
    let typed_ast :: Module = analyze(ast)
    let ir :: string = codegen(typed_ast)

    write_file("/tmp/out.ll", ir)

    -- Shell out to LLVM toolchain
    system("llc -filetype=obj /tmp/out.ll -o /tmp/out.o")
    system("clang /tmp/out.o -o " + output)

    ret 0
end
```

This avoids needing to link against LLVM libraries from null.

### Key Data Structures

```null
enum TokenKind do
    -- Literals
    EOF, IDENT, INT, FLOAT, STRING
    -- Keywords
    FN, LET, MUT, CONST, IF, ELIF, ELSE, WHILE, FOR, IN
    DO, END, RET, STRUCT, ENUM, BREAK, CONTINUE, TRUE, FALSE
    -- Operators
    PLUS, MINUS, STAR, SLASH, PERCENT
    EQ, NE, LT, LE, GT, GE
    AND, OR, NOT
    ASSIGN, DOUBLE_COLON, ARROW, DOT, COMMA
    LPAREN, RPAREN, LBRACKET, RBRACKET, LBRACE, RBRACE
    -- Directives
    AT_USE, AT_EXTERN
end

struct Token do
    kind :: TokenKind
    lexeme :: string
    line :: i64
    col :: i64
end

enum NodeKind do
    -- Expressions
    INT_LIT, FLOAT_LIT, STRING_LIT, BOOL_LIT
    IDENT, BINARY, UNARY, CALL, INDEX, FIELD
    STRUCT_LIT, ARRAY_LIT
    -- Statements
    VAR_DECL, ASSIGN, IF, WHILE, FOR, RET
    BREAK, CONTINUE, EXPR_STMT, BLOCK
    -- Declarations
    FN_DECL, STRUCT_DECL, ENUM_DECL
    USE, EXTERN_BLOCK
end

struct ASTNode do
    kind :: NodeKind
    -- Union-like fields (check kind before accessing)
    int_val :: i64
    float_val :: f64
    str_val :: string
    name :: string
    children :: [ptr<ASTNode>]
    node_type :: ptr<Type>
    line :: i64
    col :: i64
end
```

---

## Development Workflow

### Session Structure

Each session follows this loop:

1. **Read scratchpad** - What's the current state?
2. **Pick one task** - Smallest valuable increment
3. **Implement** - Write code, commit often
4. **Test** - Run tests, fix failures
5. **Update scratchpad** - Log progress, blockers, decisions

### Milestone Checklist

#### M1: Language Features ✓ when complete
- [x] Struct returns from functions
- [x] Enum types
- [x] Basic enum in match/if

#### M2: Standard Library ✓ when complete
- [x] std/mem.null (malloc/free wrappers)
- [x] std/string.null (comparison, concat, slice)
- [x] std/array.null (dynamic arrays)
- [x] std/map.null (hash map, string keys)
- [x] std/file.null (read_file, write_file)

#### M3: Lexer in null ✓ when complete
- [x] nullc/src/types.null (Token, TokenKind)
- [x] nullc/src/lexer.null (full lexer)
- [x] Lexer tests pass

#### [CURRENTLY HERE] M4: Parser in null ✓ when complete
- [ ] nullc/src/types.null (ASTNode, NodeKind)
- [ ] nullc/src/parser.null (full parser)
- [ ] Parser tests pass

#### M5: Analyzer in null ✓ when complete
- [ ] nullc/src/analyzer.null (type checking)
- [ ] Analyzer tests pass

#### M6: Codegen in null ✓ when complete
- [ ] nullc/src/codegen.null (emit LLVM IR)
- [ ] End-to-end: nullc compiles hello.null

#### M7: Self-Hosting Complete ✓ when complete
- [ ] nullc compiles itself
- [ ] nullc-compiled-by-nullc compiles hello.null
- [ ] Output matches original

---

## Research Resources

When stuck, consult:

| Topic | Resource |
|-------|----------|
| LLVM IR | https://llvm.org/docs/LangRef.html |
| LLVM struct returns | Search: "llvm sret attribute struct return" |
| Writing parsers | Crafting Interpreters (craftinginterpreters.com) |
| Compiler design | https://c9x.me/compile/ (QBE author's notes) |
| Self-hosting history | Search: "self-hosting compiler bootstrap" |

**DO NOT** guess repeatedly. If two similar attempts fail, stop and research.

---

## Success Criteria

**Phase 2 is complete when:**

1. `./nullc nullc/src/main.null -o nullc2` - nullc compiles itself
2. `./nullc2 examples/hello.null -o hello` - the compiled compiler works
3. `./hello` prints "Hello, world!"
4. All language tests pass when compiled by nullc
5. The scratchpad documents the journey

---

## Anti-Goals

Don't optimize for:
- Performance (correctness first)
- Beautiful code (working code first)
- Full feature parity with C compiler (subset is fine)
- Generics (use concrete types)
- Error recovery (fail fast is fine)

---

## Starting Point

The C compiler is complete and working:
- 8 language tests pass
- JIT, interpreter, and AOT compilation work
- REPL works
- Module system with cycle detection
- Enhanced error messages

**Next immediate task:** Implement struct returns in `src/codegen.c`

---

*The goal isn't to build a production compiler. It's to prove that AI can bootstrap a programming language from scratch. Ship something that compiles itself.*
