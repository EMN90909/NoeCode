# Noe implementation status

NoeCode is ready to try as a bootstrap language/toolchain. It is not yet a complete Noe 1.0 implementation.

## Working now

- `.noe` source files
- custom `project.noe` manifests
- no TOML for Noe projects
- lexer
- parser
- primitive static type checker
- NIR lowering
- constant-folding optimizer foundation
- NIR interpreter
- Linux x86-64 native backend for the supported subset
- assembler/linker driver
- `noe check`
- `noe nir`
- `noe run`
- `noe build`
- `noe new`
- `noe lock`
- formatter foundation
- test runner
- JSON-RPC language-server bootstrap diagnostics
- CI validation

## Supported native subset

The custom native backend currently supports integer and boolean control-flow programs, functions, local variables, arithmetic, comparisons, while loops, if/else, returns, and `print` for integers/bools and constant strings.

## Not complete yet

Roadmap features not finished yet include full records/classes/interfaces/traits, generics, multi-file modules, package registry downloads, advanced standard library, async/await, threads/channels/tasks, FFI, realtime analysis, unsafe memory/pointers/allocators, SIMD/atomics, WebAssembly, freestanding kernel/driver SDK, visual programming, and self-hosting.

## Readiness rule

Use NoeCode today for experimenting, learning the compiler architecture, writing small supported `.noe` programs, and extending the language. Do not present it as a production-ready replacement for major established languages yet.
