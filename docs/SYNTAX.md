# null Language Syntax Reference

## Philosophy

null uses a clean, minimal syntax with:
- Significant whitespace (indentation-aware)
- Block delimiters with `do`/`end` keywords
- No semicolons (newline-terminated statements)
- `::` for type annotations
- `@` prefix for compiler directives

## Comments

```null
-- single line comment
--- multi-line
    comment ---
```

## Imports

```null
@use "std/io"           -- local/std import
@use "std/io" as io     -- aliased import
@use "https://..."      -- remote import (future)
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
- `[T]` - slice/dynamic array

### Type Annotations
```null
let x :: i32 = 42
let name :: [u8] = "hello"
```

## Variables

```null
let x :: i32 = 42       -- immutable binding
mut y :: i32 = 10       -- mutable binding
let z = 100             -- type inference
```

## Functions

```null
fn add(a :: i32, b :: i32) -> i32 do
    ret a + b
end

fn greet(name :: [u8]) -> void do
    io.print(name)
end

-- short form for single expressions
fn double(x :: i32) -> i32 = x * 2
```

## Structs

```null
struct Point do
    x :: f64
    y :: f64
end

struct Person do
    name :: [u8]
    age :: i32
end

-- instantiation
let p = Point { x = 1.0, y = 2.0 }
let person = Person { name = "Alice", age = 30 }
```

## Control Flow

### If/Elif/Else
```null
if x > 0 do
    io.print("positive")
elif x < 0 do
    io.print("negative")
else do
    io.print("zero")
end
```

### While Loop
```null
mut i :: i32 = 0
while i < 10 do
    io.print(i)
    i = i + 1
end
```

### For Loop
```null
for i in 0..10 do
    io.print(i)
end

for item in collection do
    process(item)
end
```

### Match (Pattern Matching)
```null
match value do
    0 => io.print("zero")
    1 => io.print("one")
    _ => io.print("other")
end
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

### Special
- `|>` - pipe operator: `x |> fn` equals `fn(x)`
- `?` - optional unwrap (with error handling)

## Memory

```null
let p :: ptr<i32> = @alloc(i32)     -- allocate
@free(p)                             -- deallocate

-- arrays
let arr :: [i32; 5] = [1, 2, 3, 4, 5]
let slice :: [i32] = arr[1..3]       -- slicing
```

## FFI (Foreign Function Interface)

```null
@extern "C" do
    fn printf(fmt :: ptr<u8>, ...) -> i32
    fn malloc(size :: u64) -> ptr<void>
end
```

## Entry Point

Programs start at `main`:

```null
fn main() -> i32 do
    io.print("Hello, world!")
    ret 0
end
```

## Example Program

```null
@use "std/io"

struct Greeter do
    prefix :: [u8]
end

fn greet(g :: Greeter, name :: [u8]) -> void do
    io.print(g.prefix)
    io.print(name)
end

fn main() -> i32 do
    let g = Greeter { prefix = "Hello, " }
    greet(g, "world!")
    ret 0
end
```
