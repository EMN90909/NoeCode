# Native backend

Noe 0.0.8 has a custom Linux x86-64 backend. NIR is translated directly into GNU assembler syntax; the bootstrap linker driver invokes `as --64` and `ld -m elf_x86_64`.

Generated executables are freestanding ELF files. The bootstrap runtime emitted into the assembly uses Linux system calls for output and process exit. It does not depend on libc, the C++ runtime, LLVM, Rust, a Noe server, or a preinstalled Noe runtime.

## Current lowering

NIR virtual registers and named variables are assigned deterministic stack slots. User functions use the System V x86-64 integer calling convention for up to six parameters. NIR branches map to local assembly labels. Integer arithmetic lowers to native instructions including `add`, `sub`, `imul`, `idiv`, `cmp` and `setcc`.

The compiler emits small internal routines for printing C-style string constants, booleans and signed 64-bit integers using the Linux `write` syscall.

## Deliberate limits

The current backend rejects operations whose native semantics have not been implemented yet, especially floating-point code generation and runtime string concatenation. The interpreter remains the semantic reference while those lowerings are added.

Future backend work can replace GNU assembler/linker dependencies with direct ELF/object emission without changing Noe source, the AST, type checker or NIR.
