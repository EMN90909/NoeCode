# Native Backend

NoeCode includes a custom native backend for the current bootstrap subset.

## Current target

```text
Linux x86-64 ELF executable
```

The backend does not use LLVM.

## Pipeline

```text
.noe source
  -> lexer
  -> parser / AST
  -> type checker
  -> NIR
  -> optimizer
  -> Noe native backend
  -> assembly
  -> assembler/linker driver
  -> executable
```

## Build an executable

```sh
sh scripts/build.sh
./build/noe build examples/native_hello.noe build/native_hello
./build/native_hello
```

Expected output:

```text
Hello from native Noe
30
0
1
2
```

## Supported native operations

- integer constants
- boolean constants
- local variables
- integer arithmetic
- comparisons
- branches
- while loops
- function calls
- returns
- `print` for integers, booleans, and constant strings

## Explicit non-goals for this backend stage

The backend intentionally reports diagnostics instead of silently miscompiling unsupported features. Native support for floats, heap strings, arrays, records, classes, ABI-level libraries and cross-platform targets is future work.

## Next backend milestones

1. Object-file writer instead of shelling to system tools.
2. Linux ARM64 target.
3. Windows PE/COFF target.
4. macOS Mach-O target.
5. Debug information.
6. Runtime/stdlib linking.
7. FFI and calling-convention support.
8. Freestanding/system-profile output.
