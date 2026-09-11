# NoeCode

Noe is a standalone, statically typed, native-oriented general-purpose programming language. This repository contains the custom Noe bootstrap compiler and toolchain.

**Bootstrap status: phase 8/8 (`0.0.8-bootstrap`).** The bootstrap pipeline is executable end to end: source can be lexed, parsed, type-checked, lowered to NIR, optimized, interpreted, or compiled into a Linux x86-64 ELF executable.

No Rust, Cargo, TOML, or LLVM is used. Noe projects use `.noe`, `project.noe`, and `noe.lock`. The one-time bootstrap compiler is C++17. The current native backend emits Noe-owned x86-64 assembly and the linker driver invokes GNU `as`/`ld`; generated programs use Linux syscalls and do not require a Noe server or runtime installation.

## Build the bootstrap compiler

```sh
sh scripts/build.sh
./build/noe --version
```

Requirements for the bootstrap on Linux: a C++17 compiler. Native `noe build` additionally needs GNU binutils (`as` and `ld`).

## Run Noe

```sh
./build/noe check examples/native_hello.noe
./build/noe nir examples/native_hello.noe
./build/noe run examples/native_hello.noe
./build/noe build examples/native_hello.noe build/native_hello
./build/native_hello
```

Expected native output:

```text
Hello from native Noe
30
0
1
2
```

## Create a Noe project

```sh
./build/noe new hello
cd hello
../build/noe check src/main.noe
```

A generated project contains only Noe-owned project files:

```text
hello/
├── project.noe
├── noe.lock
├── src/
│   └── main.noe
└── tests/
```

`project.noe` is the human-written manifest. `noe.lock` is deterministic generated metadata. No TOML is required.

## Commands

```text
noe new <name> [dir]
noe lex <file>
noe check [file]
noe nir [file]
noe run [file]
noe build [file] [output]
noe format <file>
noe manifest [project.noe]
noe lock [project.noe]
noe test [dir]
noe lsp
```

## Compiler pipeline

```text
.noe source
   ↓
lexer
   ↓
parser / AST
   ↓
type checker
   ↓
NIR
   ↓
optimizer
   ├──────────────→ NIR interpreter
   ↓
custom x86-64 backend
   ↓
GNU assembler/linker driver
   ↓
Linux ELF executable
```

## Bootstrap components

- **Lexer** — tokens, comments, literals, keywords, operators, source spans and lexical diagnostics.
- **Parser** — recursive-descent AST parser with precedence, variables, functions, calls, blocks, `if`, `while` and `return`.
- **Type checker** — primitive inference, scopes, signatures, assignments, calls and structured `NOE-T...` diagnostics.
- **NIR** — register-based Noe Intermediate Representation with explicit control flow and calls.
- **Optimizer** — constant folding over NIR.
- **Interpreter** — reference execution path used by `noe run` and tests.
- **Native backend** — custom Linux x86-64 code generation for the current core subset.
- **Linker driver** — assembles and links freestanding ELF executables with GNU binutils.
- **Package/project manager** — custom `project.noe`, project creation and deterministic `noe.lock` generation.
- **Formatter** — canonical token-aware formatting.
- **Test runner** — discovers and executes `.noe` test programs.
- **Language server** — Content-Length framed JSON-RPC/LSP initialize flow and live compiler diagnostics for opened/changed documents.

## Native subset in 0.0.8

The native backend currently supports Linux x86-64 integer and boolean arithmetic, comparisons, variables, functions with up to six parameters, branches, loops, string constants, `print(string)`, `print(int)` and `print(bool)`. Float machine-code generation and runtime string concatenation deliberately report structured native-backend diagnostics instead of silently miscompiling.

The NIR boundary is target-independent, so Windows, ARM64, WebAssembly and future direct object/ELF writers can be added without replacing the frontend.

## Verify everything

```sh
sh scripts/test.sh
```

The end-to-end suite builds the compiler, validates a type error, exercises NIR and the interpreter, tests formatting and the test runner, builds/runs a real native executable, creates/locks a Noe project, and smoke-tests LSP initialization plus diagnostics.

Phase 8/8 means the **bootstrap toolchain milestone** is complete; it does not mean the entire long-term language specification is already Noe 1.0. Generics, records/classes, async/concurrency, full modules/registry resolution, additional native targets, systems features, visual programming and self-hosting remain later language milestones.
