# null Language Syntax Reference

## Philosophy

null uses a clean, minimal syntax with:
- Block delimiters with `do`/`end` keywords
- No semicolons (newline-terminated statements)
- `::` for type annotations
- `@` prefix for compiler directives

## Comments

```null
-- single line comment
```

## Imports

```null
@use "std/io"           -- standard library import
@use "./local_module"   -- relative import
```

Imported functions use underscore-prefixed names:
```null
@use "std/io"
io_print("hello")       -- not io.print()
```

## Types

### Primitives
- `i8`, `i16`, `i32`, `i64` - signed integers
- `u8`, `u16`, `u32`, `u64` - unsigned integers
- `f32`, `f64` - floating point
- `bool` - boolean (`true`/`false`)
- `void` - no value
- `ptr<T>` - pointer to T
- `[T; N]` - fixed array of N elements

### Type Annotations
```null
let x :: i32 = 42
let y :: i64 = 100
```

## Variables

```null
let x :: i32 = 42       -- immutable binding
mut y :: i32 = 10       -- mutable binding
let z = 100             -- type inference
```

## Functions

```null
fn add(a :: i64, b :: i64) -> i64 do
    ret a + b
end

fn greet() -> void do
    io_print("Hello!")
end
```

## Structs

```null
struct Point do
    x :: i64
    y :: i64
end

struct Person do
    name :: ptr<i8>
    age :: i32
end

-- instantiation
let p = Point { x = 10, y = 20 }

-- field access
let sum :: i64 = p.x + p.y
```

## Control Flow

### If/Elif/Else
```null
if x > 0 do
    io_print("positive")
elif x < 0 do
    io_print("negative")
else do
    io_print("zero")
end
```

### While Loop
```null
mut i :: i64 = 0
while i < 10 do
    i = i + 1
end
```

### For Loop
```null
for i in 0..10 do
    -- i goes from 0 to 9
end
```

## Arrays

```null
let arr :: [i64; 5] = [1, 2, 3, 4, 5]
let first :: i64 = arr[0]
let third :: i64 = arr[2]
```

## Operators

### Arithmetic
`+`, `-`, `*`, `/`, `%` (modulo)

### Comparison
`==`, `!=`, `<`, `>`, `<=`, `>=`

### Logical
`and`, `or`, `not`

### Bitwise
`&`, `|`, `^`, `~`, `<<`, `>>`

## FFI (Foreign Function Interface)

```null
@extern "C" do
    fn printf(fmt :: ptr<i8>, ...) -> i32
    fn puts(s :: ptr<i8>) -> i32
end
```

## Entry Point

Programs start at `main`:

```null
@use "std/io"

fn main() -> i32 do
    io_print("Hello, world!")
    ret 0
end
```

## Example Program

```null
@use "std/io"

struct Point do
    x :: i64
    y :: i64
end

fn dot(a :: Point, b :: Point) -> i64 do
    ret a.x * b.x + a.y * b.y
end

fn main() -> i32 do
    let p1 = Point { x = 3, y = 4 }
    let p2 = Point { x = 1, y = 2 }

    let result :: i64 = dot(p1, p2)
    -- result = 3*1 + 4*2 = 11

    io_print("Done!")
    ret 0
end
```

## Future Features (Not Yet Implemented)

These features are designed but not available in the current version:

- **Aliased imports**: `@use "std/io" as io`
- **Dot notation namespaces**: `io.print()` instead of `io_print()`
- **Pattern matching**: `match`/`end` blocks
- **Pipe operator**: `|>` for function chaining
- **Slices**: `[T]` dynamic arrays and `arr[1..3]` slicing
- **Multi-line comments**: `--- ... ---`
- **Short function syntax**: `fn double(x :: i32) -> i32 = x * 2`
- **Struct returns**: Returning structs from functions
