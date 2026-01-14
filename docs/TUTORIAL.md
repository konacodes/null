# null Language Tutorial

Welcome to null! This tutorial will teach you the basics of the null programming language.

## Table of Contents

1. [Installation](#installation)
2. [Hello World](#hello-world)
3. [Variables & Types](#variables--types)
4. [Functions](#functions)
5. [Control Flow](#control-flow)
6. [Loops](#loops)
7. [Structs](#structs)
8. [Arrays](#arrays)
9. [Modules](#modules)
10. [FFI (Foreign Function Interface)](#ffi)

---

## Installation

### Prerequisites
- LLVM 21+ development libraries
- Clang compiler
- Make

### Building from Source

```bash
git clone https://github.com/konacodes/null.git
cd null
make
```

### Verify Installation

```bash
./null --help
```

---

## Hello World

Create a file called `hello.null`:

```null
@use "std/io.null"

fn main() -> i32 do
    print("Hello, World!")
    ret 0
end
```

Run it:

```bash
./null hello.null
```

### Explanation

- `@use "std/io.null"` imports the standard I/O library
- `fn main() -> i32` declares the main function returning a 32-bit integer
- `do ... end` defines a code block
- `print(...)` outputs a string with a newline
- `ret 0` returns 0 (success) to the operating system

---

## Variables & Types

### Immutable Variables (let)

```null
let x :: i64 = 42       -- explicit type
let y = 100             -- type inferred as i64
let name = "Alice"      -- type inferred as ptr<u8>
```

### Mutable Variables (mut)

```null
mut counter :: i64 = 0
counter = counter + 1   -- can reassign
```

### Constants

```null
const PI :: f64 = 3.14159
const MAX_SIZE :: i64 = 1024
```

### Primitive Types

| Type | Description | Example |
|------|-------------|---------|
| `i8`, `i16`, `i32`, `i64` | Signed integers | `let x :: i64 = -42` |
| `u8`, `u16`, `u32`, `u64` | Unsigned integers | `let y :: u32 = 255` |
| `f32`, `f64` | Floating point | `let pi :: f64 = 3.14` |
| `bool` | Boolean | `let flag :: bool = true` |
| `void` | No value | Used for function returns |
| `ptr<T>` | Pointer to type T | `let s :: ptr<u8> = "hello"` |

---

## Functions

### Basic Function

```null
fn add(a :: i64, b :: i64) -> i64 do
    ret a + b
end
```

### Calling Functions

```null
let result :: i64 = add(3, 5)  -- result is 8
```

### Void Functions

```null
fn greet(name :: ptr<u8>) -> void do
    print("Hello, ")
    print(name)
end
```

---

## Control Flow

### If/Elif/Else

```null
if x > 0 do
    print("positive")
elif x < 0 do
    print("negative")
else do
    print("zero")
end
```

### Comparison Operators

| Operator | Meaning |
|----------|---------|
| `==` | Equal |
| `!=` | Not equal |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less or equal |
| `>=` | Greater or equal |

### Logical Operators

```null
if x > 0 and y > 0 do
    print("both positive")
end

if a or b do
    print("at least one true")
end

if not done do
    print("still working")
end
```

---

## Loops

### While Loop

```null
mut i :: i64 = 0
while i < 10 do
    print_int(i)
    i = i + 1
end
```

### For Loop (Range)

```null
for i in 0..10 do
    print_int(i)  -- prints 0 through 9
end
```

### Break and Continue

```null
mut i :: i64 = 0
while true do
    if i >= 5 do
        break       -- exit the loop
    end
    if i == 3 do
        i = i + 1
        continue    -- skip to next iteration
    end
    print_int(i)
    i = i + 1
end
```

---

## Structs

### Defining a Struct

```null
struct Point do
    x :: i64
    y :: i64
end
```

### Creating Instances

```null
let p = Point { x = 10, y = 20 }
```

### Accessing Fields

```null
let sum :: i64 = p.x + p.y
```

### Modifying Fields (with mut)

```null
mut p = Point { x = 0, y = 0 }
p.x = 10
p.y = 20
```

---

## Arrays

### Fixed-Size Arrays

```null
let arr :: [i64; 5] = [1, 2, 3, 4, 5]
```

### Accessing Elements

```null
let first :: i64 = arr[0]
let third :: i64 = arr[2]
```

### Modifying Elements

```null
mut arr :: [i64; 3] = [0, 0, 0]
arr[0] = 10
arr[1] = 20
arr[2] = 30
```

---

## Modules

### Importing Modules

```null
@use "std/io.null"      -- standard library
@use "./mylib.null"     -- relative path
```

### Standard Library

The standard library includes:
- `std/io.null` - Input/Output functions
- `std/math.null` - Mathematical functions

### I/O Functions

```null
@use "std/io.null"

fn main() -> i32 do
    print("Hello")          -- print with newline
    print_int(42)           -- print integer
    print_bool(true)        -- print boolean
    println()               -- print just a newline
    ret 0
end
```

---

## FFI

### Calling C Functions

```null
@extern "C" do
    fn printf(fmt :: ptr<u8>) -> i32
    fn sqrt(x :: f64) -> f64
end

fn main() -> i32 do
    printf("Hello from C!\n")
    let result :: f64 = sqrt(16.0)  -- 4.0
    ret 0
end
```

---

## Running Programs

### JIT Compilation (Default)

```bash
./null program.null
```

### Interpreter Mode

```bash
./null interp program.null
```

### Compile to Executable

```bash
./null build program.null -o program
./program
```

### Interactive REPL

```bash
./null repl
null:1> let x = 42
OK
null:2> print_int(x)
42
null:3> :exit
```

---

## Next Steps

- Explore the `examples/` directory for more code samples
- Read `docs/SYNTAX.md` for complete syntax reference
- Check `docs/GRAMMAR.md` for the formal grammar specification
- See `std/` for standard library source code

Happy coding with null!
