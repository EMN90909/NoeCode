# Freestanding Noqeri native target

Noqeri native code is defined independently of Linux and other operating systems.

## Entry contract

Freestanding x86-64 output exports `noqeri_entry(noqeri_abi*)`. It does not export `_start`, issue process syscalls, call libc, or terminate a process. The surrounding kernel, boot environment, application host, or OS adapter owns startup and shutdown.

## Noqeri ABI

`Include/noqeri/abi.h` is the Noqeri-owned ABI contract. A platform supplies callbacks for byte output, time, platform identity, and optional named services. The language and NIR remain high-level and contain no pointer arithmetic or raw-memory instructions.

## Object format and linking

The core compiler does not choose ELF, Mach-O, COFF/PE, a flat image, a kernel linker script, or another object format. `noqeri build` emits assembly. A target adapter may assemble/link that output using its own toolchain and linker script.

This keeps the native backend usable below an operating system as well as above one.