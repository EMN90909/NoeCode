# NoeCode

![Noe compiler CI](https://github.com/EMN90909/NoeCode/actions/workflows/ci.yml/badge.svg)
![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)
![Status](https://img.shields.io/badge/status-1.0.0--production--track-orange)

**NoeCode** is the bootstrap implementation of **Noe**, a standalone, statically typed, native-oriented general-purpose programming language.

Noe source files use `.noe`. Project manifests use `project.noe`. Noe projects do **not** use TOML, Cargo, Rust, or LLVM.

```noe
function add(a: int, b: int): int {
    return a + b
}

print("Hello from Noe")
print(add(10, 20))
```

## What works now

The current bootstrap is on a **1.0.0 production-track** baseline for the supported Noe subset. It is ready to clone, build, test, and use for Linux x86-64 experiments and native smoke programs.

- Lexer, parser, AST, type checker, NIR and optimizer
- Reference interpreter: `noe run`
- Custom Linux x86-64 native backend: `noe build`
- Assembler/linker driver for generated executables
- `project.noe` manifests and deterministic `noe.lock`
- `const` reassignment safety diagnostics
- Formatter foundation and `.noe` test runner
- JSON-RPC language-server bootstrap diagnostics
- Production doctor/release-check scripts
- GitHub Actions CI validation
- Public docs, MIT license, legal notice, security policy and contribution guide

## Current status

NoeCode is **usable as a production-track bootstrap compiler and language experiment**. It is not yet a complete Noe 1.0 ecosystem with every long-term feature. The native backend currently supports the core Linux x86-64 subset: integers, booleans, functions, local variables, arithmetic, comparisons, branches, while loops, returns, const assignment checks, and printing integers/bools/constant strings.

See [`docs/STATUS.md`](docs/STATUS.md) and [`docs/PRODUCTION_READINESS.md`](docs/PRODUCTION_READINESS.md) for the exact implemented feature set and limitations.

## Quick start

```sh
git clone https://github.com/EMN90909/NoeCode.git
cd NoeCode
sh scripts/build.sh
```

Run a Noe program through the interpreter:

```sh
./build/noe run examples/native_hello.noe
```

Build and run a native executable:

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

Run the complete verification suite:

```sh
sh scripts/test.sh
```

Run the production-track release check:

```sh
sh scripts/release_check.sh
```

## Project layout

```text
NoeCode/
├── bootstrap/              # C++17 bootstrap compiler implementation
├── docs/                   # user and implementer documentation
├── examples/               # runnable .noe examples
├── tests/                  # .noe compiler/runtime tests
├── scripts/                # build/test/release scripts
├── project.noe             # Noe-native project manifest
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
- [`docs/PRODUCTION_READINESS.md`](docs/PRODUCTION_READINESS.md)
- [`docs/ROADMAP.md`](docs/ROADMAP.md)

## Design direction

Noe's design goal is simple syntax without a weak language: readable everyday code, static type checking, native execution, explicit advanced systems features, and tooling that can serve text editors, visual programming, and AI agents through the same compiler model.

## License

NoeCode is licensed under the MIT License. See [`LICENSE`](LICENSE) and [`LEGAL.md`](LEGAL.md).
