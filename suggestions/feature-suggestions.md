# null Language - Feature Suggestions

**Generated:** 2026-01-13
**Status:** Recommendations for community adoption and language growth

---

## Executive Summary

The null programming language has a **solid technical foundation** with a clean, well-organized codebase. The dual execution model (compiled via LLVM + interpreted) is genuinely unique and valuable. To accelerate community adoption, focus on **developer experience** and **practical utility features** that build on existing infrastructure.

**Single Most Impactful Change:** Implement comprehensive **error messages with contextual hints**. Clear, helpful errors dramatically reduce frustration for new users.

---

## Current Strengths

1. **Clean Architecture** - Clear separation: lexer.c -> parser.c -> analyzer.c -> codegen.c/interp.c
2. **Dual Execution Model** - JIT + interpreter is unique
3. **Consistent Syntax** - `do`/`end` blocks, `::` type annotations, `@` directives
4. **Working Module System** - `@use` directive with cycle detection
5. **Good Test Coverage** - 8 language tests covering core features
6. **Documentation** - GRAMMAR.md, SYNTAX.md, MODULES.md
7. **LLVM Backend** - Professional code generation
8. **FFI Support** - `@extern` for C interop

---

## Priority A: Developer Experience (Highest Impact)

### A1. Enhanced Error Messages
**Effort:** Medium | **Impact:** Critical

Poor error messages are the #1 reason developers abandon new languages.

**Goal:**
```
Error at line 15, column 12:
  let x :: i64 = [1, 2, 3
                         ^
Expected ']' to close array literal

Note: You opened '[' at line 15, column 17
Hint: Arrays must have all elements closed: [1, 2, 3]
```

**Implementation:**
- Enhance Parser error reporting with source context
- Add error recovery to find multiple errors
- Store source lines in lexer for display
- Common error pattern detection

---

### A2. REPL (Read-Eval-Print Loop)
**Effort:** Medium | **Impact:** High

Testing snippets requires creating files. REPLs are essential for exploration.

**Goal:**
```bash
$ ./null repl
null> let x = 42
null> x + 10
52
null> fn double(n :: i64) -> i64 do ret n * 2 end
null> double(5)
10
```

**Implementation:**
- Add `repl` command in main.c
- Leverage existing interpreter (interp.c)
- Maintain persistent state between inputs
- Add special commands (:help, :exit, :type)

---

### A3. Complete Standard Library I/O
**Effort:** Low-Medium | **Impact:** High

**Add to std/io.null:**
```null
fn print_u64(n :: u64) -> void
fn print_f64(n :: f64) -> void
fn read_line() -> ptr<u8>
fn read_file(path :: ptr<u8>) -> ptr<u8>
fn write_file(path :: ptr<u8>, content :: ptr<u8>) -> i32
fn file_exists(path :: ptr<u8>) -> bool
```

---

### A4. Debugging Aids
**Effort:** Medium | **Impact:** High

**Add:**
1. **Assertions:** `@assert(condition, "message")`
2. **Debug print:** `@debug(value)` shows file:line:value
3. **Stack traces** on runtime errors
4. **Verbose mode:** `./null --verbose program.null`

---

## Priority B: Language Features

### B1. Struct Returns from Functions
**Effort:** Medium | **Impact:** Critical

Currently structs cannot be returned from functions, which severely limits their utility.

**Goal:**
```null
fn create_point(x :: i64, y :: i64) -> Point do
    ret Point { x = x, y = y }
end
```

**Technical:** Update codegen.c to handle struct return types using LLVM struct return conventions.

---

### B2. Break and Continue
**Effort:** Low-Medium | **Impact:** Medium

Common control flow patterns require awkward workarounds without these.

```null
while condition do
    if early_exit do
        break
    end
    if skip_iteration do
        continue
    end
end
```

---

### B3. Constants
**Effort:** Low-Medium | **Impact:** Medium

```null
const PI :: f64 = 3.14159
const MAX_SIZE :: i64 = 1024
```

---

### B4. Enum Types
**Effort:** High | **Impact:** Medium

Start with integer-backed enums:
```null
enum Color do
    Red     -- 0
    Green   -- 1
    Blue    -- 2
end

let c :: Color = Color.Red
```

---

### B5. Match/Switch Expression
**Effort:** Medium | **Impact:** Medium

```null
let result = match value do
    1 -> "one"
    2 -> "two"
    _ -> "other"
end
```

---

## Priority C: Tooling & Ecosystem

### C1. Syntax Highlighting
**Effort:** Low-Medium | **Impact:** High

**Priority:** VS Code -> Vim -> Others

Create TextMate grammar for VS Code:
- Keywords, types, strings, comments
- Bracket matching for do/end
- Basic snippets

**Files:** `editors/vscode/`, `editors/vim/`

---

### C2. Build System / Project Structure
**Effort:** Medium | **Impact:** Medium

```bash
null new my-app        # Create project scaffold
null build             # Build current project
null test              # Run tests
```

---

### C3. Language Server Protocol (LSP)
**Effort:** High | **Impact:** Very High

Start with:
1. Diagnostics (show errors inline)
2. Autocomplete (variables, functions, fields)
3. Go to definition
4. Hover information

---

## Priority D: Documentation & Community

### D1. Tutorial / Getting Started Guide
**Effort:** Low | **Impact:** Critical

**Create:** `docs/TUTORIAL.md` with:
1. Installation
2. Hello World
3. Variables & Types
4. Functions
5. Control Flow
6. Structs
7. Arrays
8. Modules
9. FFI
10. Building Programs

---

### D2. More Examples
**Effort:** Low-Medium | **Impact:** Medium

```
examples/
├── algorithms/
│   ├── quicksort.null
│   ├── binary_search.null
│   └── prime_sieve.null
├── data_structures/
│   ├── linked_list.null
│   └── stack.null
└── games/
    └── tic_tac_toe.null
```

---

### D3. Community Infrastructure
**Effort:** Low | **Impact:** High

**Create:**
- `CONTRIBUTING.md` - How to contribute
- `CODE_OF_CONDUCT.md` - Community standards
- `LICENSE` file (MIT)
- `CHANGELOG.md` - Version history
- `ROADMAP.md` - Future plans
- `.github/ISSUE_TEMPLATE/` - Bug/feature templates

---

### D4. API Reference Documentation
**Effort:** Low | **Impact:** Medium

- `docs/api/std-io.md`
- `docs/api/std-math.md`

---

## Quick Wins (Under 30 Minutes Each)

1. Add badges to README (build status, license)
2. Create LICENSE file
3. Add CHANGELOG.md
4. Create issue templates
5. Add example GIF showing compilation
6. Create examples/README.md
7. Add smoke test script
8. Add installation script
9. Create CONTRIBUTING.md skeleton
10. Add .gitattributes for GitHub linguist

---

## Implementation Priority (Top 10)

1. **Enhanced error messages** - Biggest UX improvement
2. **Complete I/O functions** - Makes language usable
3. **REPL** - Essential for exploration
4. **Tutorial documentation** - Onboarding path
5. **Struct returns** - Removes major limitation
6. **Break/continue** - Expected control flow
7. **VS Code syntax highlighting** - First impression
8. **More examples** - Learning material
9. **CONTRIBUTING.md** - Enable contributors
10. **Constants** - Quality of life

---

## Strategic Positioning

### Target Niche: Education
- Clean syntax, both compiled + interpreted, strong types
- Compete with Python for teaching but with better type safety
- Simple enough to learn, powerful enough to use

### Marketing Angle
The "built with Claude" story is unique and compelling:
- Interest from AI community
- Case study for AI-assisted development
- Unique positioning in language ecosystem

---

## Summary

The null programming language has excellent fundamentals. To unlock community growth:

1. **Fix DX friction points** (errors, I/O, REPL)
2. **Document the journey** (tutorial, examples, API docs)
3. **Enable contribution** (CONTRIBUTING.md, good first issues)
4. **Pick a niche** and optimize for it

The best feature is one that removes friction for new users. Prioritize anything that makes the first 10 minutes with null delightful.
