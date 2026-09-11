# Platform support

The compiler frontend, parser, type checker, NIR, interpreter and ABI library are C++17 and are CI-tested on Linux, macOS and Windows.

## Native code

The native backend targets **freestanding x86-64 Noqeri ABI v2**, not Linux x86-64. Generated assembly contains no Linux syscalls, `_start`, ELF loader contract, libc call, or process-exit sequence.

The emitted program exports `noqeri_entry(noqeri_abi*)`. A surrounding platform adapter chooses how that entry is loaded and called.

Object/executable format is intentionally outside the core compiler:

- Linux may use ELF tooling.
- macOS may use Mach-O tooling.
- Windows may use COFF/PE tooling and an ABI shim.
- a kernel may use a custom linker script, relocatable object flow, flat image, or its own loader.

`NOQERI_NATIVE_ASSEMBLER` configures the optional assembly-to-object adapter. No object format or OS linker is selected implicitly.
