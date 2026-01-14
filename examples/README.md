# null Language Examples

This directory contains example programs demonstrating various features of the null programming language.

## Running Examples

```bash
# JIT compile and run
./null examples/hello.null

# Interpret
./null interp examples/hello.null

# Compile to executable
./null build examples/hello.null -o hello
./hello
```

## Example Files

| File | Description |
|------|-------------|
| `hello.null` | Hello World - the classic first program |
| `fibonacci.null` | Fibonacci sequence calculation |
| `vector_math.null` | Vector operations using structs |

## Creating Your Own Examples

1. Create a new file with `.null` extension
2. Import the standard library: `@use "std/io.null"`
3. Define a `main` function returning `i32`
4. Use `print()` or `print_int()` for output

### Template

```null
@use "std/io.null"

fn main() -> i32 do
    print("Hello from my example!")
    ret 0
end
```

## Tips

- Use comments (`--`) to document your code
- Use the REPL (`./null repl`) to test snippets quickly
- Check error messages - they include hints for common mistakes

## Contributing Examples

We welcome example contributions! Good examples:
- Demonstrate a single concept clearly
- Include comments explaining what's happening
- Are short and self-contained
- Work with both JIT and interpreter modes

See `CONTRIBUTING.md` for contribution guidelines.
