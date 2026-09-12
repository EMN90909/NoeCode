<p align="center">
  <img src="Brand/noqeri-logo.webp" alt="Noqeri code logo" width="560">
</p>

<h1 align="center">Noqeri</h1>
<p align="center"><strong>A compact statically typed language for applications, systems, portable web objects and embedded data.</strong></p>

<p align="center">
  <img alt="Noqeri CI" src="https://github.com/EMN90909/Noqeri/actions/workflows/ci.yml/badge.svg">
  <img alt="GPL-3.0-only license" src="https://img.shields.io/badge/license-GPL--3.0--only-536dfe.svg">
  <img alt="source extension" src="https://img.shields.io/badge/source-.nqr-102654.svg">
  <img alt="language" src="https://img.shields.io/badge/language-1.0-536dfe.svg">
</p>

**Noqeri 1.0** is the current language/toolchain line. Source files use **`.nqr`**, project manifests use **`project.nqr`**, lock files use **`noqeri.lock`**, portable web objects use **`.nqo`**, NoqeriDB scripts use **`.nqd`**, and embedded database files use **`.nqdb`**.

```nqr
module demo

function max<T: Ord>(left: T, right: T): T {
    if left > right { return left }
    return right
}

let values: [int; 4] = [1, 2, 3, 4]
let view: []int = slice(values)
print(max(view[1], view[3]))
```

## First build

Noqeri still has a C++ bootstrap compiler, so a source build needs **CMake** and a working **C++ toolchain**. The build scripts now detect those prerequisites before attempting the build.

### Windows PowerShell

```powershell
git clone https://github.com/EMN90909/Noqeri.git
cd Noqeri
.\bootstrap.ps1
```

You can also run:

```powershell
.\scripts\build.ps1
```

If CMake or a C++ toolchain is missing, Noqeri lists everything missing and asks once:

```text
Do you want to install all missing prerequisites now? [Y/N]
```

`Y` installs the missing Windows prerequisites through **winget**. For the C++ toolchain, the script uses Visual Studio 2022 Build Tools with the C++ workload. `N` installs nothing, reports that the build was skipped, and exits without the old `cmake is not recognized` crash.

To approve installation non-interactively:

```powershell
.\bootstrap.ps1 -Yes
```

To prohibit installation explicitly:

```powershell
.\bootstrap.ps1 -SkipInstall
```

After the build, the script finds the generated compiler whether CMake used a single-config layout or the Visual Studio `build\Release\` layout and runs `noqeri --version` automatically.

### Linux

```sh
git clone https://github.com/EMN90909/Noqeri.git
cd Noqeri
./scripts/build.sh
```

If required tools are missing, the script can install them with a supported system package manager after the same Y/N approval. Supported Linux package-manager paths include apt, dnf, zypper, pacman and apk.

### macOS

```sh
git clone https://github.com/EMN90909/Noqeri.git
cd Noqeri
./scripts/build.sh
```

CMake can be installed through Homebrew when approved. If Apple Command Line Tools are missing, the script opens Apple's installer and asks you to rerun after installation completes.

Both build scripts display the canonical terminal wordmark from `Brand/noqeri-banner.txt`:

```text
███╗   ██╗ ██████╗  ██████╗ ███████╗██████╗ ██╗
████╗  ██║██╔═══██╗██╔═══██╗██╔════╝██╔══██╗██║
██╔██╗ ██║██║   ██║██║   ██║█████╗  ██████╔╝██║
██║╚██╗██║██║   ██║██║▄▄ ██║██╔══╝  ██╔══██╗██║
██║ ╚████║╚██████╔╝╚██████╔╝███████╗██║  ██║██║
╚═╝  ╚═══╝ ╚═════╝  ╚══▀▀═╝ ╚══════╝╚═╝  ╚═╝╚═╝
```

### Manual CMake build

On a machine where CMake and a C++ compiler are already installed, the ordinary single-config sequence remains:

```sh
git clone https://github.com/EMN90909/Noqeri.git
cd Noqeri
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/noqeri --version
```

A freshly cloned repository **cannot intercept a missing bare `cmake` command**: the shell resolves `cmake` before any Noqeri script can run. That is why first-time Windows users should use `bootstrap.ps1` or `scripts/build.ps1` once.

With the default Visual Studio generator on Windows, use the explicit configuration form if you build manually:

```powershell
cmake -S . -B build
cmake --build build --config Release --parallel
.\build\Release\noqeri.exe --version
```

## What is implemented today

The C++17 bootstrap compiler currently includes:

- lexer, parser and AST;
- static type checking with explicit narrowing and signedness rules;
- fixed-width integers, arrays, borrowed slices, records and raw pointers;
- slice escape analysis for known local-array escapes;
- constrained function generics including `Ord`, `Numeric`, `Integer`, `Eq` and `Copy`;
- generic function monomorphization with an instantiation cache;
- typed NIR values, basic blocks and CFG successor information;
- a linear-scan allocation planner;
- optimization passes, a reference interpreter and freestanding x86-64 assembly generation;
- atomics, volatile memory, target intrinsics and constrained inline assembly;
- modules/import graphs, formatter, diagnostics, project manifests, SHA-256 locks, audit tooling and an LSP foundation;
- portable `.nqo` web-object generation;
- the embedded NoqeriDB `.nqd` / `.nqdb` compatibility engine.

CI builds and tests the compiler on Linux, macOS and Windows, runs sanitizers, exercises freestanding native output, runs semantic regressions, executes the full local Noqeri library smoke program, tests the registry service and compiles every published package entrypoint with the current compiler.

## Noqeri-first implementation

The bootstrap compiler is still C++, but reusable language-facing behavior is increasingly written in Noqeri.

Current `.nqr` implementation areas include:

- `Lib/std/` — integer, slice, search, sorting, statistics, bytes, UTF-8, checksums, memory, status, time, random, matrix, range, text, set, map, stack, queue, algorithms, bit operations, validation, windows, pairs, counters and comparison helpers;
- `Lib/security/` — memory clearing and validation helpers;
- `Lib/test/` — reusable assertions;
- `Modules/` and `Objects/` — built-in/native-module helpers and value/text primitives;
- `Database/noqeridb.nqr` — fixed-capacity database-core algorithms;
- `Parser/`, `Runtime/`, `Programs/`, `Platforms/`, `PC/`, `Mac/`, `Android/` and `PCbuild/` — Noqeri-side parser/runtime/program/platform policy and helpers;
- `Tools/security/`, `Tools/fuzz/` and `Tools/build/` — reusable policy and generation logic.

The remaining C++ host/bootstrap sources are deliberately isolated under `Compiler/` and `Compiler/bootstrap/`. The repository verifiers reject C/C++ files leaking back into Noqeri-owned implementation directories.

Run the complete local library smoke program with:

```sh
./build/noqeri run examples/ecosystem_smoke.nqr
```

It imports the full local standard/security/test library surface and exercises representative algorithms and data structures.

## Packages and registry

The official registry is maintained at [EMN90909/noqeri-registry](https://github.com/EMN90909/noqeri-registry).

The 1.0 catalog includes core/std/security/test plus JSON, HTTP contracts, HTTP server contracts, URL, HTML, web, DOM, fetch, routing, filesystem/environment, crypto contracts, NoqeriDB access, time and WebSockets. PostgreSQL, SQLite interoperability, raw networking and TLS are provider-contract packages and remain experimental where the underlying provider is not implemented by Noqeri itself.

The compiler CI now checks the ecosystem from both directions:

1. build the current Noqeri compiler;
2. run `examples/ecosystem_smoke.nqr` against local libraries;
3. check out `noqeri-registry`;
4. run the registry Go tests and vet checks;
5. compile every published 1.0 package entrypoint using that exact compiler build.

Packages are distributed as deterministic `.nqpkg` archives with SHA-256 identities. The current compiler understands project manifests and lock integrity, but it does **not** yet claim a complete network dependency resolver/downloader. Registry package compilation in CI should not be confused with automatic package installation on an end-user machine.

## Portable web objects (`.nqo`)

`noqeri web` compiles checked Noqeri source into a portable ES-module-compatible artifact:

```sh
noqeri web src/api.nqr build/api.nqo
```

Native-only operations such as raw assembly are rejected by the portable web backend. Browser/server environment capabilities still depend on host/provider bindings where documented.

## NoqeriDB (`.nqd` / `.nqdb`)

NoqeriDB is a small embedded typed database included in the toolchain. The current CLI path supports `int`, `real`, `bool` and `text` columns, required/unique/key constraints, table creation, insert/select/update/delete, whole-script persistence and duplicate key/unique rejection.

```nqd
table users {
    id: int key,
    name: text required,
    active: bool required
}

insert users { id: 1, name: "Ada", active: true }
select users where active = true
```

Run it with:

```sh
noqeri db schema.nqd app.nqdb
```

NoqeriDB is not currently presented as a SQLite or PostgreSQL replacement.

## Verification

Unix:

```sh
./scripts/verify_repository.sh
./scripts/test.sh
```

Windows:

```powershell
.\scripts\verify_repository.ps1
ctest --test-dir build -C Release --output-on-failure
```

Useful compiler commands include:

```text
noqeri --version
noqeri new <name> [directory]
noqeri lex <file.nqr>
noqeri check [file.nqr]
noqeri nir [file.nqr]
noqeri run [file.nqr]
noqeri build [file.nqr] [assembly]
noqeri web [file.nqr] [module.nqo]
noqeri db <script.nqd> [data.nqdb]
noqeri assemble <assembly> <object> [target]
noqeri link <output> <object...>
noqeri targets
noqeri format <file.nqr>
noqeri manifest [project.nqr]
noqeri lock [project.nqr]
noqeri audit [directory]
noqeri test [directory]
noqeri doctor [directory]
noqeri release-check [directory]
noqeri lsp
```

## Current limits

Noqeri 1.0 is the current project API line, not a claim of ecosystem parity with long-established languages. Important limits include:

- the compiler/runtime is not self-hosted yet;
- native object writing is not yet implemented directly; the native backend emits x86-64 assembly and uses assembler/linker adapters;
- the allocation planner is not yet fully integrated into the established x86 emitter;
- typed NIR/basic blocks exist, but canonical SSA with PHI insertion is incomplete;
- the LSP foundation lacks several richer IDE operations;
- the package manager does not yet provide a complete network resolver/download/cache pipeline;
- PostgreSQL, TLS, raw networking and similar package surfaces remain provider contracts until their providers are completed;
- NoqeriDB remains intentionally small.

## Repository map

| Area | Responsibility |
|---|---|
| `Compiler/` | bootstrap checking, generics, NIR, optimization and target backends |
| `Compiler/bootstrap/` | C++ lexer/parser/runtime/CLI/LSP/package/host adapters still needed during self-hosting |
| `Parser/`, `Runtime/`, `Programs/` | Noqeri-side reusable parser/runtime/program helpers |
| `Lib/` | Noqeri standard, security and test libraries |
| `Modules/`, `Objects/`, `Database/` | Noqeri language/runtime/data implementation |
| `Platforms/`, `PC/`, `Mac/`, `Android/`, `PCbuild/` | Noqeri platform policy/capability source |
| `Tools/` | Noqeri security/build/fuzz policy plus repository scripts |
| `tests/`, `Benchmarks/`, `examples/` | conformance, performance seeds and runnable programs |
| `Include/noqeri/` | public bootstrap compiler/database/ABI APIs |
| `Doc/`, `InternalDocs/`, `Grammar/` | language, ABI and compiler documentation |
| `editors/` | editor integrations |
| `Brand/` | canonical visual and terminal assets |

## License

Noqeri is distributed under **GNU GPL v3 only (GPL-3.0-only)**. See [`LICENSE`](LICENSE).

Made by [**Noethric**](https://noethric.xyz).
