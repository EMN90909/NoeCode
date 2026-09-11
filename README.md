<p align="center">
  <img src="docs/assets/noe-logo.svg" alt="Noe wolf code logo" width="420">
</p>

# NoeCode

![Noe compiler CI](https://github.com/EMN90909/NoeCode/actions/workflows/ci.yml/badge.svg)
![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)
![Status](https://img.shields.io/badge/status-1.0.0--production--track-orange)
![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20Windows%20%7C%20macOS-blueviolet)

**NoeCode** is the production-track bootstrap implementation of **Noe**, a standalone, statically typed, native-oriented general-purpose programming language.

Noe source files use `.noe`. Project manifests use `project.noe`. Noe projects do **not** use TOML, Cargo, Rust, Python, or LLVM.

```noe
function add(a: int, b: int): int {
    return a + b
}

print("Hello from Noe")
print(add(10, 20))
```

## What works now

The current codebase is organized as a real language implementation, inspired by the completeness of mature interpreter/compiler repositories while staying original to Noe.

- Lexer, parser, AST, type checker, NIR and optimizer
- Reference interpreter: `noe run`
- Custom Linux x86-64 native backend: `noe build`
- Assembler/linker driver for generated Linux ELF executables
- `project.noe` manifests and deterministic `noe.lock`
- `const` reassignment safety diagnostics
- Formatter foundation and `.noe` test runner
- JSON-RPC language-server bootstrap diagnostics
- Cross-platform bootstrap compiler build on Linux, Windows and macOS
- Production doctor/release-check scripts
- GitHub Actions CI validation
- Public docs, MIT license, legal notice, security policy and contribution guide

## Platform support

| Platform | Build compiler | `noe check` | `noe run` interpreter | `noe lsp` | Native executable backend |
|---|---:|---:|---:|---:|---:|
| Linux x86-64 | ✅ | ✅ | ✅ | ✅ | ✅ ELF via custom backend |
| macOS | ✅ | ✅ | ✅ | ✅ | ⚠️ planned |
| Windows | ✅ | ✅ | ✅ | ✅ | ⚠️ planned |

Windows and macOS are ready for compiler/frontend usage, diagnostics, formatting, tests, project tooling and interpreted `.noe` execution. The current native executable backend is intentionally limited to Linux x86-64 until the Windows PE/COFF and macOS Mach-O backends are implemented.

See [`docs/PLATFORM_SUPPORT.md`](docs/PLATFORM_SUPPORT.md) for exact platform behavior.

## Quick start

Linux/macOS:

```sh
git clone https://github.com/EMN90909/NoeCode.git
cd NoeCode
sh scripts/build.sh
sh scripts/test_core.sh
```

Windows PowerShell:

```powershell
git clone https://github.com/EMN90909/NoeCode.git
cd NoeCode
.\scripts\build.ps1
.\scripts\test_core.ps1
```

Run a Noe program through the interpreter:

```sh
./build/noe run examples/native_hello.noe
```

Build and run a native executable on Linux x86-64:

```sh
./build/noe build examples/native_hello.noe build/native_hello
./build/native_hello
```

Expected output:

```text
Hello from native Noe
30
0
1
2
```

Run the complete Linux release verification suite:

```sh
sh scripts/release_check.sh
```

## Repository layout

```text
NoeCode/
├── compiler/bootstrap/      # C++17 one-time bootstrap compiler implementation
├── include/noe/             # public bootstrap compiler header/API surface
├── grammar/                 # Noe grammar and syntax notes
├── stdlib/std/              # Noe standard-library seed modules
├── runtime/                 # runtime design and future runtime components
├── platforms/               # Linux, Windows and macOS target notes
├── tools/                   # developer tooling area
├── docs/                    # user and implementer documentation
├── examples/                # runnable .noe examples
├── tests/                   # .noe compiler/runtime tests
├── scripts/                 # build/test/release scripts
├── project.noe              # Noe-native project manifest
├── CMakeLists.txt           # optional cross-platform C++ bootstrap build
├── Makefile                 # simple POSIX convenience build
├── LICENSE
└── LEGAL.md
```

A Noe app looks like this:

```text
myapp/
├── project.noe
├── src/
│   └── main.noe
└── noe.lock
```

## CLI

```text
noe --version
noe new <name>
noe check <file>
noe lex <file>
noe nir <file>
noe run <file>
noe build <file> <output>
noe format <file>
noe manifest [project.noe]
noe lock [project.noe]
noe test [directory]
noe doctor [directory]
noe release-check [directory]
noe lsp
```

## Documentation

Start at [`docs/README.md`](docs/README.md).

Important pages:

- [`docs/GETTING_STARTED.md`](docs/GETTING_STARTED.md)
- [`docs/LANGUAGE_REFERENCE.md`](docs/LANGUAGE_REFERENCE.md)
- [`docs/TOOLCHAIN.md`](docs/TOOLCHAIN.md)
- [`docs/NATIVE_BACKEND.md`](docs/NATIVE_BACKEND.md)
- [`docs/PROJECTS.md`](docs/PROJECTS.md)
- [`docs/STATUS.md`](docs/STATUS.md)
- [`docs/PLATFORM_SUPPORT.md`](docs/PLATFORM_SUPPORT.md)
- [`docs/CPYTHON_STRUCTURE_REVIEW.md`](docs/CPYTHON_STRUCTURE_REVIEW.md)
- [`docs/PRODUCTION_READINESS.md`](docs/PRODUCTION_READINESS.md)
- [`docs/ROADMAP.md`](docs/ROADMAP.md)

## Design direction

Noe's design goal is simple syntax without a weak language: readable everyday code, static type checking, native execution, explicit advanced systems features, and tooling that can serve text editors, visual programming, and AI agents through the same compiler model.

## License

NoeCode is licensed under the MIT License. See [`LICENSE`](LICENSE) and [`LEGAL.md`](LEGAL.md).
