<p align="center">
  <img src="Doc/_static/ric-logo.svg" alt="Ric wolf code logo" width="520">
</p>

<h1 align="center">Ric</h1>
<p align="center"><strong>A statically typed, native-oriented general-purpose programming language.</strong></p>

<p align="center">
  <img alt="Ric CI" src="https://github.com/EMN90909/NoeCode/actions/workflows/ci.yml/badge.svg">
  <img alt="License MIT" src="https://img.shields.io/badge/license-MIT-4c6fff.svg">
  <img alt="language Ric" src="https://img.shields.io/badge/source-.ric-101b3d.svg">
  <img alt="platforms" src="https://img.shields.io/badge/bootstrap-Linux%20%7C%20Windows%20%7C%20macOS-4c6fff.svg">
</p>

**Ric** is the renamed successor to the Noe language in this repository. The public language name is **Ric**, source files use **`.ric`**, project manifests use **`project.ric`**, lock files use **`ric.lock`**, and the command-line tool is **`ric`**.

The repository is organized after the engineering lessons of mature language runtimes such as CPython: grammar, public headers, parser/compiler internals, runtime/object documentation, standard library, platform build areas, tools, tests, docs, editor integration, CI and release policy all have explicit homes. This is an architectural-completeness target, not a claim of CPython feature parity.

```ric
function add(a: int, b: int): int {
    return a + b
}

print("Hello from Ric")
print(add(10, 20))
```

## Current implementation

The checked-in C++17 bootstrap implements the Ric 1.0 production-track core: lexer, parser, AST, static type checking, NIR lowering, optimizer, interpreter, formatter, project/lock tooling, test discovery and a JSON-RPC language-server foundation. Linux x86-64 also has the current native assembly backend and linker driver. Windows and macOS build and run the frontend/interpreter/tooling while native PE/COFF and Mach-O emission remain future backend work.

## Quick start

Linux or macOS:

```sh
git clone https://github.com/EMN90909/NoeCode.git
cd NoeCode
./scripts/build.sh
./build/ric run examples/hello.ric
```

Windows PowerShell:

```powershell
git clone https://github.com/EMN90909/NoeCode.git
cd NoeCode
.\scripts\build.ps1
.\build\Release\ric.exe run examples\hello.ric
```

Run the repository checks:

```sh
./scripts/test_core.sh
./scripts/release_check.sh
```

## CLI

```text
ric --version
ric new <name> [directory]
ric lex <file.ric>
ric check [file.ric]
ric nir [file.ric]
ric run [file.ric]
ric build [file.ric] [output]
ric format <file.ric>
ric manifest [project.ric]
ric lock [project.ric]
ric test [directory]
ric doctor [directory]
ric release-check [directory]
ric lsp
```

## Repository map

| Area | Responsibility |
|---|---|
| `Grammar/` | Ric syntax and grammar contract |
| `Include/ric/` | public bootstrap compiler API |
| `Parser/` | parser architecture and parser ownership |
| `Python/` | compiler pipeline/core implementation map, analogous to CPython's core-runtime area |
| `Objects/` | Ric runtime value/object model |
| `Modules/` | built-in/native module boundary |
| `Lib/` | Ric standard-library sources |
| `Programs/` | CLI program entry points and command contract |
| `Runtime/` | execution/runtime architecture |
| `Tools/` | developer and release tooling |
| `Doc/` | user and implementer documentation |
| `InternalDocs/` | compiler internals and CPython structure study |
| `PC/`, `PCbuild/`, `Mac/`, `Android/` | platform integration/build notes |
| `editors/` | VS Code/TextMate and IDE integration |
| `site/` | public static project site, using the Ric logo |
| `compiler/bootstrap/` | buildable C++17 bootstrap implementation |
| `examples/`, `tests/` | runnable `.ric` programs and verification cases |

## IDE support

`editors/vscode/` contains a complete local VS Code language package: `.ric` registration, bracket/comment rules, TextMate syntax highlighting and a Ric file-icon theme using the wolf/code mark. The same TextMate grammar can be imported by editors that support TextMate bundles. The compiler's `ric lsp` command provides the language-server foundation.

## Project format

A Ric project is deliberately simple:

```text
myapp/
├── project.ric
├── ric.lock
├── src/
│   └── main.ric
└── tests/
    └── basic.ric
```

Ric does not require TOML, Cargo, Python, Rust or LLVM to describe a Ric project.

## Documentation

Start with [`Doc/README.md`](Doc/README.md), then read the [`language reference`](Doc/LANGUAGE_REFERENCE.md), [`toolchain`](Doc/TOOLCHAIN.md), [`platform support`](Doc/PLATFORM_SUPPORT.md), and [`production readiness`](Doc/PRODUCTION_READINESS.md). Repository architecture and the CPython comparison are in [`InternalDocs/CPYTHON_STRUCTURE.md`](InternalDocs/CPYTHON_STRUCTURE.md).

## Status and versioning

The language surface is named **Ric 1.0**. The bootstrap implementation is intentionally marked **production-track** until the native backends, module/import semantics, self-hosting and broader conformance suite meet the release gates documented in `Doc/PRODUCTION_READINESS.md`.

## License

MIT. See [`LICENSE`](LICENSE) and [`LEGAL.md`](LEGAL.md).
