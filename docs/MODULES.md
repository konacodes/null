# null Module System

## Overview

null uses a simple, extensible module system based on the `@use` directive. The design prioritizes:

1. **Simplicity** - Easy to understand and use
2. **Locality** - Local imports are straightforward
3. **Extensibility** - Can be extended for remote imports
4. **Caching** - Modules are parsed once and cached

## Import Syntax

```null
@use "std/io"              -- Import std library module
@use "std/io" as io        -- Import with alias
@use "./local_module"      -- Import relative to current file
@use "../sibling/module"   -- Import from parent directory
```

## Module Resolution

### Standard Library

Modules starting with `std/` are resolved from the standard library:

1. First, check `$NULL_STD_PATH` environment variable
2. Then, check `./std/` relative to current directory
3. Finally, check the bundled standard library location

### Local Modules

Modules starting with `./` or `../` are resolved relative to the importing file:

```null
-- In src/main.null
@use "./utils"           -- Resolves to src/utils.null
@use "../lib/helpers"    -- Resolves to lib/helpers.null
```

### Absolute Paths

Modules can use absolute paths:

```null
@use "/home/user/mylib/module"
```

## Module Interface

Each `.null` file is a module. By default, all top-level declarations are private. Use `pub` to export:

```null
-- math.null
pub fn add(a :: i32, b :: i32) -> i32 do
    ret a + b
end

fn internal_helper() -> void do
    -- This is private
end
```

## Namespace Access

Imported modules create a namespace:

```null
@use "std/io"

fn main() -> i32 do
    io.print("Hello!")  -- Access via module name
    ret 0
end
```

With aliases:

```null
@use "std/io" as output

fn main() -> i32 do
    output.print("Hello!")
    ret 0
end
```

## Module Caching

Modules are parsed and compiled once per compilation unit. If multiple files import the same module, it's only processed once.

## Future: Remote Imports

The module system is designed to support remote imports in the future:

```null
-- Not yet implemented
@use "https://example.com/packages/mylib@1.0.0"
@use "github.com/user/repo/module"
```

Remote modules would be:
1. Downloaded to a local cache (`~/.null/cache/`)
2. Verified via checksums or lock files
3. Compiled and linked normally

## Implementation Details

### Internal Representation

Modules are tracked in the compiler as:

```c
typedef struct Module {
    char *path;          // Original import path
    char *resolved_path; // Actual file path
    char *alias;         // Optional alias
    ASTNode *ast;        // Parsed AST
    bool is_compiled;    // Has been through codegen
} Module;
```

### Name Mangling

Module functions are name-mangled to avoid conflicts:

```
std/io.print -> std_io_print
mymodule.helper -> mymodule_helper
```

This allows multiple modules to have functions with the same name.

### Circular Imports

Circular imports are detected and result in a compile error:

```null
-- a.null
@use "./b"  -- Error if b.null imports a.null

-- b.null
@use "./a"  -- Creates circular dependency
```

## Best Practices

1. **Use the standard library** - Prefer `std/` modules over reimplementing
2. **Keep modules focused** - One responsibility per module
3. **Use aliases sparingly** - Only when avoiding name conflicts
4. **Organize by feature** - Group related modules in directories
