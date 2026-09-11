<p align="center">
  <img src="Brand/noqeri-logo.webp" alt="noqeri wolf code logo" width="560">
</p>

<h1 align="center">noqeri</h1>
<p align="center"><strong>A statically typed, native-oriented general-purpose programming language.</strong></p>

<p align="center">
  <img alt="noqeri CI" src="https://github.com/EMN90909/Noqeri/actions/workflows/ci.yml/badge.svg">
  <img alt="MIT license" src="https://img.shields.io/badge/license-MIT-536dfe.svg">
  <img alt="source extension" src="https://img.shields.io/badge/source-.nqr-102654.svg">
  <img alt="language" src="https://img.shields.io/badge/language-1.4-536dfe.svg">
</p>

**noqeri** is the language name. Source files use **`.nqr`**, project manifests use **`project.nqr`**, lock files use **`noqeri.lock`**, and the command-line compiler is **`noqeri`**.

Noqeri keeps common programs compact while making low-level intent explicit when needed. Systems capabilities are ordinary general-purpose language features; there is no special kernel mode, OS dialect, Linux runtime assumption, or mandatory host ABI path.

```nqr
module demo

function identity<T>(value: T): T {
    return value
}

let values: [int; 4] = [1, 2, 3, 4]
let view: []int = slice(values)
print(identity(view[2]))
```

## Implemented today

The C++17 bootstrap contains a lexer/parser/AST, static type checker, NIR, optimizer, reference interpreter, formatter, project tooling, tests, LSP foundation, diagnostics, a Noqeri-owned ABI v2, and a freestanding x86-64 native backend.

The stable 1.4 surface includes:

- ordinary functions, values, control flow, arithmetic and calls;
- `u8/u16/u32/u64`, `i8/i16/i32/i64`, `usize/isize`;
- pointers, `&value`, dereference, pointer indexing, casts and volatile access;
- deterministic `record` layout plus `extern` / `export` functions;
- fixed arrays `[T; N]`, array literals and slices `[]T`;
- `module` declarations and recursive relative `import "file.nqr"` graphs;
- inferred register-value function generics;
- allocation-free `try` / `throw` integer status propagation;
- typed atomic load/store/exchange/compare-exchange/fence operations;
- explicit CPU intrinsics and constrained single-instruction inline assembly.

The x86-64 backend emits freestanding assembly with `noqeri_entry(noqeri_abi*)`; it does not embed Linux syscalls, `_start`, ELF linker invocation, or an OS process model. Platform object formats, boot paths, application hosts and kernels remain integration layers outside the language.

## Quick start

Linux or macOS:

```sh
git clone https://github.com/EMN90909/Noqeri.git
cd Noqeri
./scripts/build.sh
./build/noqeri run examples/hello.nqr
```

Windows PowerShell:

```powershell
git clone https://github.com/EMN90909/Noqeri.git
cd Noqeri
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

The freestanding native release path is exercised with:

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
noqeri build [file.nqr] [assembly]
noqeri assemble <assembly> <object>
noqeri format <file.nqr>
noqeri manifest [project.nqr]
noqeri lock [project.nqr]
noqeri test [directory]
noqeri doctor [directory]
noqeri release-check [directory]
noqeri lsp
```

`check`, `nir`, `run`, and `build` compile a file plus its recursively imported source graph. `lex` intentionally operates on one source file.

## Repository map

| Area | Responsibility |
|---|---|
| `Grammar/` | implemented syntax contract |
| `Include/noqeri/` | public compiler and ABI API |
| `Parser/` | lexer/parser implementation |
| `Compiler/` | type checking, NIR, optimization and native code generation |
| `Runtime/` | reference interpreter and ABI bridge |
| `Objects/` | runtime value/object representation contracts |
| `Modules/` | module/import ownership and future namespace evolution |
| `Lib/` | standard-library `.nqr` source and library tests |
| `Programs/` | CLI, formatter, LSP, package and test tooling |
| `Tools/` | build, fuzz, production and maintenance tooling |
| `tests/`, `Benchmarks/`, `examples/` | conformance, performance seeds and runnable programs |
| `Doc/` | user-facing docs |
| `InternalDocs/` | compiler/repository internals and testing policy |
| `Platforms/`, `PC/`, `PCbuild/`, `Mac/`, `Android/` | optional platform-specific integration |
| `Misc/` | release/project records |
| `editors/` | VS Code, JetBrains/TextMate, Vim/Neovim and Sublime integration |
| `site/` | public static project site |
| `Brand/` | canonical noqeri logo and compact editor mark |

## Project format

```text
myapp/
├── project.nqr
├── noqeri.lock
├── src/
│   ├── main.nqr
│   └── math.nqr
└── tests/
    └── basic.nqr
```

Example import:

```nqr
module app.main
import "math.nqr"

print(twice(4))
```

Noqeri does not require TOML, Cargo, Rust, Python or LLVM to describe or compile a Noqeri project.

## Documentation and status

Start at [`Doc/README.md`](Doc/README.md), then see [`Doc/LANGUAGE_REFERENCE.md`](Doc/LANGUAGE_REFERENCE.md), [`Doc/ABI.md`](Doc/ABI.md), and [`Doc/FREESTANDING.md`](Doc/FREESTANDING.md).

The language surface is **Noqeri 1.4 general systems**. The implemented subset is kept buildable and regression-tested across the bootstrap platforms; feature maturity and broader target work remain separate from the language contract.

## License

MIT. See [`LICENSE`](LICENSE) and [`LEGAL.md`](LEGAL.md).
