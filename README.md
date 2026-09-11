# NoeCode

Noe is a standalone, statically typed, native-oriented general-purpose programming language. This repository contains the custom Noe compiler/toolchain bootstrap.

No Rust is used. Noe projects do not use TOML: source files are `.noe`, project metadata is `project.noe`, and future dependency locks use `noe.lock`.

## Current milestone: phase 3/8

This milestone turns the repository into an executable compiler foundation rather than documentation only. The lexer, parser, type checker, NIR lowering, basic optimizer and reference interpreter are implemented and wired together. The package-manifest reader, formatter foundation, test runner and language-server boundary are also present.

The native backend and linker driver are intentionally explicit boundaries at this phase; they return structured diagnostics instead of pretending native code generation is complete. Custom machine-code emission is the next major milestone.

## Build the bootstrap compiler

Linux/macOS:

```sh
sh scripts/build.sh
```

Windows PowerShell with MSVC or g++:

```powershell
./scripts/build.ps1
```

The one-time bootstrap requires a C++17 compiler. That is only for bootstrapping Noe itself; Noe programs are not Rust projects and do not use Cargo or TOML. The long-term compiler will be progressively rewritten in `.noe` until it can compile itself.

## Try Noe

```sh
./build/noe check examples/hello.noe
./build/noe nir examples/hello.noe
./build/noe run examples/hello.noe
./build/noe test tests
```

Expected output from the example:

```text
Hello Noe
30
```

## Compiler pipeline

```text
.noe source
  -> lexer
  -> parser / AST
  -> type checker
  -> NIR
  -> optimizer
  -> interpreter        (working now)
  -> native backend     (next milestone)
  -> linker driver      (next milestone)
```

## Toolchain modules

- Lexer: source text to tokens with spans and structured lexical diagnostics.
- Parser: recursive-descent parser with precedence handling and AST construction.
- Type checker: primitive inference, lexical scopes, typed functions and assignment/call checking.
- NIR: register-based Noe Intermediate Representation with control flow and calls.
- Optimizer: constant-folding foundation over NIR.
- Interpreter: executes NIR, including functions, variables, loops, branches and `print`.
- Native backend: isolated interface ready for custom target code generation.
- Linker driver: isolated link stage ready for target object formats.
- Package manager: reads custom `project.noe` manifests; remote resolution comes later.
- Formatter: token-based canonical formatting foundation.
- Test runner: discovers and executes `.noe` test programs.
- Language server: compiler-service boundary ready for later JSON-RPC/LSP transport.

See `docs/BOOTSTRAP.md` for the exact phase boundary.
