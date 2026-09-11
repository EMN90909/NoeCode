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

Integer constants, boolean constants, local variables, integer arithmetic, comparisons, branches, while loops, function calls, returns, and `print` for integers, booleans, and constant strings.

## Explicit limitations

Native support for floats, heap strings, arrays, records, classes, ABI-level libraries and cross-platform targets is future work.
