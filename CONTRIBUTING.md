# Contributing to null

Thank you for your interest in contributing to the null programming language! This document provides guidelines for contributing.

## Ways to Contribute

- **Report bugs** - Open an issue with a clear description and reproduction steps
- **Suggest features** - Open an issue describing the feature and its use cases
- **Improve documentation** - Fix typos, clarify explanations, add examples
- **Write code** - Fix bugs, implement features, improve performance
- **Write tests** - Add test cases for existing or new features

## Getting Started

### Prerequisites

- LLVM 21+ development libraries
- Clang compiler
- Make

### Building

```bash
git clone https://github.com/yourusername/null.git
cd null
make
```

### Running Tests

```bash
make test
```

### Project Structure

```
null/
├── src/           # Compiler source code
│   ├── lexer.c    # Tokenization
│   ├── parser.c   # Parsing
│   ├── analyzer.c # Semantic analysis
│   ├── codegen.c  # LLVM code generation
│   └── interp.c   # Tree-walking interpreter
├── std/           # Standard library
├── tests/         # Test suite
├── docs/          # Documentation
└── examples/      # Example programs
```

## Development Workflow

1. **Fork** the repository
2. **Create a branch** for your feature/fix: `git checkout -b feature-name`
3. **Make your changes**
4. **Test** your changes: `make test`
5. **Commit** with a clear message
6. **Push** to your fork
7. **Open a Pull Request**

## Code Style

### C Code

- Use 4-space indentation
- Keep lines under 100 characters
- Use descriptive variable names
- Comment complex logic
- Follow existing patterns in the codebase

### null Code

- Use 4-space indentation
- Use snake_case for functions and variables
- Use PascalCase for struct names
- Add comments for non-obvious code

## Commit Messages

Write clear, concise commit messages:

```
Add break/continue statements to while loops

- Add TOK_BREAK and TOK_CONTINUE tokens
- Parse break and continue in statement context
- Implement codegen with LLVM branch instructions
- Add interpreter support
```

## Testing

### Adding Tests

Add test files to `tests/lang/` with the `.null` extension. Tests should:
- Have descriptive names
- Cover edge cases
- Return 0 on success, non-zero on failure

Example test structure:

```null
-- tests/lang/my_feature.null

fn main() -> i32 do
    -- Test case 1
    if not test_case_1() do ret 1 end

    -- Test case 2
    if not test_case_2() do ret 2 end

    ret 0  -- All tests passed
end
```

## Reporting Bugs

When reporting bugs, please include:

1. **Description** - What happened vs. what you expected
2. **Reproduction steps** - Minimal code to reproduce
3. **Environment** - OS, LLVM version, etc.
4. **Error messages** - Full error output

## Feature Requests

When suggesting features:

1. **Description** - What the feature does
2. **Use cases** - Why it's useful
3. **Examples** - How it would look in code
4. **Alternatives** - Other ways to achieve the goal

## Code of Conduct

Be respectful and inclusive. We welcome contributors of all backgrounds and experience levels.

## Questions?

Open an issue or discussion if you have questions about contributing.

Thank you for helping make null better!
