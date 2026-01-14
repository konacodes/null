# null Module System

## Overview

null uses a simple module system based on the `@use` directive. The design prioritizes:

1. **Simplicity** - Easy to understand and use
2. **Locality** - Local imports are straightforward
3. **Flat namespacing** - Module functions use underscore prefixes

## Import Syntax

```null
@use "std/io"              -- Import std library module
@use "./local_module"      -- Import relative to current file
```

## Using Imported Functions

Imported modules expose functions with an underscore-prefixed naming convention:

```null
@use "std/io"

fn main() -> i32 do
    io_print("Hello, world!")   -- std/io functions use io_ prefix
    io_println("With newline")
    ret 0
end
```

```null
@use "std/math"

fn main() -> i32 do
    let a :: i64 = abs(-42)           -- math functions are global
    let b :: i64 = max(10, 20)
    ret 0
end
```

## Module Resolution

### Standard Library

Modules starting with `std/` are resolved from the standard library:

1. Check `./std/` relative to current directory
2. Check the compiler's bundled standard library location

### Local Modules

Modules starting with `./` or `../` are resolved relative to the importing file:

```null
-- In src/main.null
@use "./utils"           -- Resolves to src/utils.null
@use "../lib/helpers"    -- Resolves to lib/helpers.null
```

## Standard Library Modules

### std/io

I/O operations for printing to stdout.

```null
@use "std/io"

io_print("text")      -- Print without newline
io_println("text")    -- Print with newline
```

### std/math

Mathematical functions.

```null
@use "std/math"

abs(n)                    -- Absolute value (i64)
min(a, b)                 -- Minimum of two i64
max(a, b)                 -- Maximum of two i64
clamp(val, min, max)      -- Clamp value to range

-- C math library (f64):
sqrt(x)
pow(base, exp)
sin(x), cos(x), tan(x)
log(x), exp(x)
floor(x), ceil(x)
fabs(x)
```

## How It Works

The `@use` directive tells the compiler to include extern declarations for the module's functions. For `std/io`, this adds:

```null
@extern "C" do
    fn puts(s :: ptr<i8>) -> i32
end
```

The `io_print` and `io_println` functions are wrappers that call these C functions.

## Future Enhancements

These features are designed but not yet implemented:

- **Aliases**: `@use "std/io" as output`
- **Dot notation**: `io.print("hello")`
- **Visibility**: `pub fn` for exports
- **Remote imports**: `@use "github.com/user/repo"`

## Best Practices

1. **Use the standard library** - Prefer `std/` modules over reimplementing
2. **Keep modules focused** - One responsibility per module
3. **Organize by feature** - Group related modules in directories
