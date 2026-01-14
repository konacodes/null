# null - Language Specification

## Philosophy

You are building `null`, a compiled programming language written in C. This spec defines *what* to build, not *how*. Make implementation decisions yourself. When you encounter errors or get stuck in circles, **stop and research** using web search or documentation rather than guessing repeatedly. The syntax should be unique, yet readable. snazzy yet familiar. Be different!

## Core Principle: Minimal Kernel

The compiler core should contain **only primitives** - the absolute minimum needed to bootstrap everything else. All other functionality lives in the standard library. Think of it like a microkernel: the compiler knows how to handle memory, basic types, function calls, and module loading. Everything else - even basic operations like printing, string manipulation, math beyond arithmetic - should be library code written in null itself or thin C bindings.

Ask yourself: "Can this be a library instead of a primitive?" If yes, make it a library.

## Final Deliverable

A single binary called `null` that works like this:

```bash
./null program.null      # compiles and runs program.null
./null run program.null  # same as above
./null build program.null -o program  # compile to executable
```

The binary should handle the full pipeline: lex → parse → analyze → codegen → link → (optionally) execute.

## Module System

Design a creative and extensible module import system. Consider:

- Local imports (relative paths, project-local modules)
- Remote imports (URLs, package registry, git repos?)
- How dependencies are resolved and cached
- How the standard library is distributed (bundled? fetched? embedded?)

Some inspiration (don't copy, innovate):
- Go's URL-based imports
- Deno's URL imports with lock files  
- Zig's package manager
- A content-addressed approach?

The module system should make it trivial to extend null's capabilities without modifying the compiler. Document your design in `docs/MODULES.md` as you build it.

## Primitives (Compiler Core)

These are suggestions. Adjust based on what you discover is truly necessary:

- Integer types (at minimum: i64, u64, or similar)
- Floating point (f64)
- Booleans
- Pointers/references
- Functions
- Structs/records
- Arrays (fixed or dynamic - your call)
- Module/import mechanism
- Basic control flow (if, loops, return)

Everything else = library.

## Standard Library Structure

Create a `std/` directory with null source files. Start minimal:

```
std/
  io.null       # print, read, file operations
  string.null   # string manipulation
  math.null     # beyond basic arithmetic
  mem.null      # allocators, utilities
  ...
```

These can call into C via whatever FFI mechanism you design.

## Testing Strategy

Maintain two test suites:

### 1. Compiler Tests (`tests/compiler/`)
Unit tests for the C codebase. Test lexer, parser, codegen in isolation.

```bash
make test-compiler  # or however you structure it
```

### 2. Language Tests (`tests/lang/`)
Programs written in null that verify language behavior:

```
tests/lang/
  arithmetic.null     # 1 + 1 == 2
  functions.null      # function calls work
  modules.null        # imports resolve correctly
  std_io.null         # std/io works
  ...
```

Each test file should be runnable and self-verifying (exit 0 = pass, exit 1 = fail, or use assertions).

```bash
./null test tests/lang/  # run all language tests
```

### Test-Driven Loop

When adding features:
1. Write a failing test in `tests/lang/`
2. Implement until it passes
3. Refactor
4. Commit

## Development Loop Instructions

You are operating in a self-improving loop. On each iteration:

1. **Assess**: What's the current state? What works? What's broken?
2. **Prioritize**: What's the smallest next step that adds value?
3. **Implement**: Build it. Write tests.
4. **Verify**: Run tests. Does it work?
5. **Document**: Update docs if needed.

### When Stuck

**DO NOT** repeatedly try variations of the same fix. If something fails twice with similar approaches:

1. Stop
2. Research the error message or concept
3. Read documentation or examples
4. Understand *why* it's failing before trying again

Use web search. Read man pages. Check how other compilers solve similar problems. This is not cheating - it's engineering.

## Build System

Use Make, CMake, or just a shell script - your choice. The build should:

- Compile all C sources into the `null` binary
- Be incremental (don't rebuild everything every time)
- Have a `clean` target
- Have a `test` target that runs both test suites

## Code Organization

Suggested (adapt as needed):

```
null/
  src/
    main.c          # entry point, CLI handling
    lexer.c/.h      # tokenization  
    parser.c/.h     # AST construction
    analyzer.c/.h   # type checking, semantic analysis
    codegen.c/.h    # code generation (to C? to asm? LLVM? your call)
    runtime.c/.h    # minimal runtime if needed
  std/              # standard library (null source files)
  tests/
    compiler/       # C unit tests
    lang/           # null language tests
  docs/
    MODULES.md      # module system documentation
    SYNTAX.md       # language syntax reference
  Makefile
  SPEC.md           # this file
```

## Codegen Target

Decide how null compiles to machine code:

- **Transpile to C**: Easiest. Generate C, then call gcc/clang. (NOT PREFERRED)
- **Direct assembly**: More control, more work. (SOME DAY)
- **LLVM**: Powerful optimization, steeper learning curve. (PREFERRED)
- **Custom bytecode + interpreter**: Simpler but not truly "compiled."

Pick one and commit. You can always revisit later.

## Syntax

Design the syntax yourself. Make it:

- Consistent
- Unambiguous (easy to parse)
- Reasonably ergonomic

Document it in `docs/SYNTAX.md` as you go. Start simple - you can add sugar later.

## Non-Goals (For Now)

Don't worry about these initially:

- Garbage collection (manual memory or arena allocators are fine)
- Generics/templates
- Async/concurrency
- REPL
- IDE integration
- Performance optimization

Get it working first. Make it good later.

## Success Criteria

You're done with the initial version when:

1. `./null examples/hello.null` prints "Hello, world!" and exits
2. The hello world program uses `std/io` for printing
3. At least 5 language tests pass
4. The module system can import local files
5. A program can define and call functions

Everything beyond this is iteration.

---

*Remember: Research when stuck. Primitives stay primitive. Libraries do the work. Ship something that runs.*
