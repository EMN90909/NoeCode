# Noe Roadmap

NoeCode is moving from bootstrap compiler to a serious general-purpose language.

## Done: bootstrap foundation

- lexer, parser, AST, primitive type checking
- NIR lowering and optimizer foundation
- interpreter
- CLI
- custom Linux x86-64 backend for the supported subset
- assembler/linker driver
- native executable generation
- project creation and deterministic lockfiles
- language-server diagnostics
- CI smoke tests
- public documentation/legal foundation

## 0.2: language core expansion

Multi-file modules/imports, arrays/lists, records, richer type checking, `const` mutation enforcement, better diagnostics, and initial standard library.

## 0.3: application language

Classes, interfaces, generics, result/optional ergonomics, package registry design, formatter maturity, and language-server completion/hover/go-to-definition.

## 0.4: runtime and interop

Strings and heap values in native backend, runtime/stdlib linking, C ABI FFI, improved cross-platform targets.

## 0.5: concurrency and tooling

Async/await, task scopes, channels, testing framework, debugger hooks, generated docs from compiler metadata.

## 1.0: stable application Noe

Stable syntax and semantics for application programming, reproducible builds, usable package ecosystem, standard library core, and native compiler on major desktop/server platforms.

## Beyond 1.0

Visual programming frontend, Noe Engine bindings, realtime profile diagnostics, unsafe systems profile, atomics/SIMD/custom allocators, freestanding builds, driver/kernel SDK, and self-hosted compiler.
