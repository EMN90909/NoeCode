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

These are roadmap features, not finished features:

- records/classes/interfaces/traits as full semantic systems
- generics
- modules/import resolver across many files
- package registry downloads
- advanced standard library
- async/await
- threads/channels/tasks
- C/C++/C# FFI
- realtime profile analysis
- unsafe memory/pointers/allocators
- SIMD/atomics
- WebAssembly backend
- freestanding kernel/driver SDK
- visual programming frontend
- self-hosted Noe compiler

## Readiness rule

Use NoeCode today for experimenting, learning the compiler architecture, writing small supported `.noe` programs, and extending the language. Do not present it as a production-ready replacement for C#, C++, Rust, Go, JavaScript, or Python yet.
