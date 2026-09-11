<p align="center">
  <img src="Brand/noqeri-logo.webp" alt="noqeri wolf code logo" width="560">
</p>

<h1 align="center">noqeri</h1>
<p align="center"><strong>A statically typed, native-oriented general-purpose programming language.</strong></p>

<p align="center">
  <img alt="noqeri CI" src="https://github.com/EMN90909/NoeCode/actions/workflows/ci.yml/badge.svg">
  <img alt="MIT license" src="https://img.shields.io/badge/license-MIT-536dfe.svg">
  <img alt="source extension" src="https://img.shields.io/badge/source-.nqr-102654.svg">
  <img alt="platforms" src="https://img.shields.io/badge/bootstrap-Linux%20%7C%20Windows%20%7C%20macOS-536dfe.svg">
</p>

**noqeri** is the language name. Source files use **`.nqr`**, project manifests use **`project.nqr`**, lock files use **`noqeri.lock`**, and the command-line compiler is **`noqeri`**. The wolf/code artwork above is the canonical project identity supplied for noqeri; a compact derivative of the same artwork is used for `.nqr` editor and file icons.

The repository follows the maintainability lesson visible in CPython: grammar, public headers, parser, compiler, runtime, object/module boundaries, standard library, platform integration, programs, tools, tests, benchmarks, documentation, editor support, CI and release policy each have an explicit home. This is an architecture and repository-ownership model, **not a claim of CPython feature parity or maturity**.

```nqr
function add(a: int, b: int): int {
    return a + b
}

print("Hello from noqeri")
print(add(10, 20))
```

## Implemented today

The C++17 bootstrap contains a lexer, parser and AST, static type checker, NIR lowering, optimizer, reference interpreter, formatter, project/lock tooling, test runner, JSON-RPC language-server foundation, diagnostics, and a custom Linux x86-64 assembly/backend-linker path. Linux, Windows and macOS build the frontend/interpreter/tooling; native PE/COFF and Mach-O emission remain release-track work.

The implementation is physically split by responsibility: `Parser/` owns lexing/parsing, `Compiler/` owns semantic/NIR/native compilation, `Runtime/` owns execution, `Programs/` owns user-facing tools, and `Tools/` owns production/release machinery.

## Quick start

Linux or macOS:

```sh
git clone https://github.com/EMN90909/NoeCode.git
cd NoeCode
./scripts/build.sh
./build/noqeri run examples/hello.nqr
```

Windows PowerShell:

```powershell
git clone https://github.com/EMN90909/NoeCode.git
cd NoeCode
.\scripts\build.ps1
.\build\Release\noqeri.exe run examples\hello.nqr
```

Core verification:

```sh
./scripts/verify_repository.sh
./scripts/test_core.sh
```

Install with CMake through the provided wrappers:

```sh
./scripts/install.sh
```

On Linux x86-64, the native release path is exercised with:

```sh
./scripts/release_check.sh
```

## CLI

```text
noqeri --version
noqeri new <name> [directory]
noqeri lex <file.nqr>
noqeri check [file.nqr]
noqeri nir [file.nqr]
noqeri run [file.nqr]
noqeri build [file.nqr] [output]
noqeri format <file.nqr>
noqeri manifest [project.nqr]
noqeri lock [project.nqr]
noqeri test [directory]
noqeri doctor [directory]
noqeri release-check [directory]
noqeri lsp
```

## Repository map

| Area | Responsibility |
|---|---|
| `Grammar/` | implemented syntax contract |
| `Include/noqeri/` | public bootstrap compiler API |
| `Parser/` | lexer/parser implementation and parser ownership |
| `Compiler/` | semantic checking, NIR, optimization and native compilation |
| `Runtime/` | interpreter, native-runtime and ABI design |
| `Objects/` | runtime value/object representation contracts |
| `Modules/` | built-in/native module boundary |
| `Lib/` | standard-library `.nqr` source and library tests |
| `Programs/` | CLI, formatter, LSP, package and test tooling |
| `Tools/` | build, fuzz, production and maintenance tooling |
| `tests/`, `Benchmarks/`, `examples/` | conformance, performance seeds and runnable programs |
| `Doc/` | user-facing docs |
| `InternalDocs/` | compiler/repository internals and testing policy |
| `Platforms/`, `PC/`, `PCbuild/`, `Mac/`, `Android/` | platform-specific integration |
| `Misc/` | release/project records |
| `editors/` | VS Code, JetBrains/TextMate, Vim/Neovim and Sublime integration |
| `site/` | public static project site |
| `Brand/` | canonical noqeri logo and compact editor mark |

## Editor and IDE support

`editors/vscode/` registers `.nqr`, TextMate highlighting, comments/brackets, snippets and a file-icon theme using the noqeri wolf/code mark. `editors/jetbrains/` documents TextMate/LSP setup. Vim/Neovim filetype + syntax files and a Sublime syntax definition are included. The compiler's `noqeri lsp` command is the language-server foundation.

## Project format

```text
myapp/
├── project.nqr
├── noqeri.lock
├── src/
│   └── main.nqr
└── tests/
    └── basic.nqr
```

noqeri does not require TOML, Cargo, Rust, Python or LLVM to describe a noqeri project.

## Documentation and status

Start at [`Doc/README.md`](Doc/README.md). Repository architecture and the CPython comparison are documented in [`InternalDocs/CPYTHON_STRUCTURE.md`](InternalDocs/CPYTHON_STRUCTURE.md). Testing ownership is in [`InternalDocs/TESTING.md`](InternalDocs/TESTING.md).

The language surface is **noqeri 1.0 production-track**. That means the current subset is kept buildable and tested; it does not mean every planned systems-language feature is finished. Remaining gates are tracked in [`Doc/PRODUCTION_READINESS.md`](Doc/PRODUCTION_READINESS.md).

## License

MIT. See [`LICENSE`](LICENSE) and [`LEGAL.md`](LEGAL.md).
