# Noe Roadmap

NoeCode is moving from bootstrap compiler to a serious general-purpose language.

## Phase 1-3: compiler foundation — done

- lexer
- parser
- AST
- primitive type checking
- NIR lowering
- optimizer foundation
- interpreter
- CLI
- formatter/test/LSP/package boundaries

## Phase 4-8: executable bootstrap — done

- custom Linux x86-64 backend for the supported subset
- assembler/linker driver
- native executable generation
- project creation
- deterministic lockfiles
- language-server diagnostics
- CI smoke tests
- public documentation/legal foundation

## 0.2: language core expansion

- multi-file modules/imports
- arrays/lists
- records
- richer type checking
- `const` mutation enforcement
- better diagnostics with source snippets
- initial standard library written in Noe where possible

## 0.3: application language

- classes
- interfaces
- generics
- result/optional ergonomics
- package registry design
- formatter maturity
- language-server completion/hover/go-to-definition

## 0.4: runtime and systems bridge

- strings and heap values in native backend
- runtime/stdlib linking
- C ABI FFI
- improved cross-platform targets

## 0.5: concurrency and tooling

- async/await design implementation
- task scopes
- channels
- testing framework
- debugger hooks
- generated docs from compiler metadata

## 1.0: stable application Noe

- stable syntax and semantics for application programming
- reproducible builds
- usable package ecosystem
- stable standard library core
- native compiler on major desktop/server platforms

## Beyond 1.0

- visual programming frontend sharing NIR
- Noe Engine bindings
- realtime profile diagnostics
- unsafe systems profile
- atomics/SIMD/custom allocators
- freestanding builds
- driver/kernel SDK
- self-hosted compiler
