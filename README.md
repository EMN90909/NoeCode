<p align="center">
  <img src="Brand/noqeri-logo.webp" alt="Noqeri fox code logo" width="560">
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

Noqeri keeps ordinary code familiar while making low-level intent visible when it is actually needed. Raw pointers, volatile memory, atomics, intrinsics and constrained inline assembly remain available for systems work; application code does not need a separate language or dialect.

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

## What is implemented today

The C++17 bootstrap compiler currently includes:

- lexer, parser and AST;
- static type checking with explicit narrowing/signedness rules;
- fixed-width integers, arrays, borrowed slices, records and raw pointers;
- slice escape analysis for known local-array escapes;
- constrained function generics such as `T: Ord`, `Numeric`, `Integer`, `Eq` and `Copy`;
- concrete per-type generic function monomorphization with an instantiation cache;
- typed NIR values, basic blocks and CFG successor information;
- a linear-scan allocation planner; the x86-64 correctness backend still uses its established stack representation rather than claiming full allocator integration;
- optimization passes, a reference interpreter and freestanding x86-64 assembly generation;
- atomics, volatile memory, target intrinsics and deliberately constrained inline assembly;
- modules/import graphs, formatter, diagnostics, project manifests, SHA-256 locks, audit tooling and an LSP foundation;
- portable **`.nqo`** web-object generation;
- the embedded **NoqeriDB** `.nqd` / `.nqdb` compatibility engine.

The CI matrix builds and tests the compiler on Linux, macOS and Windows, runs sanitizers, exercises freestanding native output and runs semantic regressions.

## Noqeri-first implementation

The bootstrap is still C++, but reusable non-compiler logic is now being moved into Noqeri source instead of leaving implementation folders as Markdown roadmaps.

Current Noqeri-owned components include:

- `Modules/builtin.nqr` — executable helpers around actual built-in language operations such as `len`, slices and typed indices;
- `Modules/native.nqr` — native-module ABI/API compatibility and status rules;
- `Objects/value.nqr` — ABI-compatible value-tag/header primitives;
- `Objects/text.nqr` — byte-string equality, ordering and prefix operations;
- `Database/noqeridb.nqr` — executable fixed-capacity database-core lookup/insert/update/delete/read algorithms;
- `Tools/security/policy.nqr` — reusable range/index and full-scan byte validation helpers;
- `Lib/test/assert.nqr` — source-level Noqeri assertions.

`tests/noqeri_owned_components.nqr` imports these components together and runs them through the actual compiler and reference runtime. They are source code, not future-feature Markdown placeholders.

C++ remains where it is currently necessary to bootstrap Noqeri: parsing/type checking/code generation, the reference NIR interpreter, the ABI bridge, and host adapters that need filesystem/process/platform access. The remaining `.nqd` grammar/persistent `.nqdb` file adapter is isolated at `Compiler/bootstrap/noqeridb_host.cpp`, while the repository/package security scanner is isolated at `Compiler/bootstrap/security_audit_host.cpp`. `Database/`, `Modules/`, `Objects/`, `Lib/`, and `Tools/security/` are now Noqeri-owned implementation areas. Removing those host adapters before Noqeri has equivalent typed filesystem capabilities would regress working behavior, so the boundary is kept explicit instead of making a false self-hosting claim.

The repository verifiers and `noqeri doctor` enforce this rule: C/C++ source is rejected inside those Noqeri-owned areas and the old Markdown-only implementation placeholders are forbidden from returning.

The architectural rule going forward is: **library/policy/algorithm code in Noqeri first; C/C++ only at the compiler/bootstrap/host edge when the language cannot yet perform the required environment operation.**

## Portable web objects (`.nqo`)

`noqeri web` compiles checked Noqeri source into a portable ES-module-compatible `.nqo` artifact:

```sh
noqeri web src/api.nqr build/api.nqo
```

The current portable runtime exposes browser-facing JSON, URL, HTML escaping, routing, DOM, fetch, WebSocket and time helpers plus explicit host capabilities for HTTP/static serving, filesystem, environment, crypto and database access.

The host boundary is intentional. Noqeri source decides application behavior while a browser or server host provides environment-specific capabilities. Raw pointers, inline assembly and native-only intrinsics are rejected by the portable web backend.

`.nqo` is not a new JavaScript runtime or a claim that every Node/browser API is native Noqeri syntax. It is the current portable application target for checked Noqeri programs.

## NoqeriDB (`.nqd` / `.nqdb`)

NoqeriDB is a small embedded typed database included in the toolchain. The source-level database core now lives in `Database/noqeridb.nqr`; the bootstrap compatibility adapter at `Compiler/bootstrap/noqeridb_host.cpp` keeps the established `.nqd` parser and `.nqdb` persistence working while more database behavior moves into Noqeri.

The current CLI path supports:

- `int`, `real`, `bool` and `text` columns;
- `required`, `unique` and `key` constraints;
- `table`, `insert`, `select`, `update` and `delete` statements;
- atomic whole-script persistence through a temporary-file commit;
- reopening `.nqdb` files across separate CLI executions;
- duplicate key/unique rejection.

The Noqeri-owned core is separately exercised for key lookup, insert, duplicate rejection, update, delete and read operations.

Example:

```nqd
table users {
    id: int key,
    name: text required,
    active: bool required
}

insert users { id: 1, name: "Ada", active: true }
update users set { name: "Ada Lovelace" } where id = 1
select users where active = true
```

Run it with:

```sh
noqeri db schema.nqd app.nqdb
```

NoqeriDB is **not** currently presented as a SQLite replacement on performance, durability or concurrency. It does not yet have a WAL, production-grade concurrent transactions, a query planner or mature indexes. Those require implementation and benchmarks before such claims are justified.

## Packages and registry

The official registry is maintained separately at [EMN90909/noqeri-registry](https://github.com/EMN90909/noqeri-registry). The 1.0 catalog includes application-facing packages for core/std, JSON, HTTP contracts, routing, DOM, fetch, filesystem/environment, crypto contracts, DB access, time and WebSockets, plus provider contracts for PostgreSQL, SQLite interoperability, raw networking and TLS.

Registry CI builds the current Noqeri compiler and type-checks every published 1.0 package entrypoint. Provider-contract packages are still labelled experimental where the underlying transport/provider is not implemented by Noqeri itself.

Packages are distributed as deterministic `.nqpkg` archives with SHA-256 identities. The package ecosystem is growing; package names are not treated as proof of production maturity.

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

Both build scripts print the Noqeri fox mark before configuring the bootstrap compiler.

Core verification:

```sh
./scripts/verify_repository.sh
./scripts/test.sh
```

The semantic suite includes positive and negative type-safety cases, constrained generics, string escaping, `.nqo` generation, Noqeri-owned built-in/native modules, object/security/database algorithms, NoqeriDB CRUD/persistence/constraint checks, formatter preservation, package locking and native assembly validation.

## CLI

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

`check`, `nir`, `run`, `build` and `web` compile a file plus its recursively imported source graph. `lex` intentionally operates on one source file.

## Current limits

Noqeri 1.0 should be read as the current project API line, not as a claim of ecosystem parity with long-established languages. Important limits include:

- the compiler/runtime is not self-hosted yet; C++ remains the bootstrap frontend, reference runtime and host-adapter layer;
- the `.nqd` grammar/persistent `.nqdb` file adapter still uses bootstrap C++ while the database core migrates into Noqeri;
- native machine-code/object writing is not yet implemented; the native backend emits x86-64 assembly and uses explicit assembler/linker adapters;
- the current register allocator produces allocation plans but the established x86 emitter has not fully switched from its stack-backed virtual-register representation;
- typed NIR/basic blocks exist, but canonical SSA with PHI insertion is not complete;
- the LSP has project-aware diagnostics/overlays and formatting, while richer completion/rename/reference features remain incomplete;
- PostgreSQL, TLS, raw networking and similar package surfaces are provider contracts until their underlying providers are completed;
- NoqeriDB is intentionally small today and is not benchmarked as a replacement for SQLite/PostgreSQL.

These limits are kept explicit so documentation tracks executable behavior instead of roadmap promises.

## Repository map

| Area | Responsibility |
|---|---|
| `Grammar/` | implemented syntax contract |
| `Include/noqeri/` | public compiler, database and ABI APIs |
| `Parser/` | bootstrap lexer/parser implementation |
| `Compiler/` | bootstrap checking, generics, NIR, optimization and target backends |
| `Compiler/bootstrap/` | narrow C++ host/compatibility adapters still needed during self-hosting |
| `Runtime/` | reference interpreter and ABI bridge |
| `Modules/` | Noqeri built-in helpers and native-module contracts |
| `Objects/` | Noqeri value and byte/text model |
| `Lib/` | Noqeri standard/test library source |
| `Database/` | Noqeri database-core source |
| `Programs/` | CLI, formatter, LSP, package and test host tooling |
| `Tools/security/` | Noqeri security-policy source |
| `Tools/build/` | bootstrap build/production checks |
| `tests/`, `Benchmarks/`, `examples/` | conformance, performance seeds and runnable programs |
| `Doc/` | user-facing language/toolchain documentation |
| `InternalDocs/` | compiler/repository internals and testing policy |
| `Platforms/`, `PC/`, `PCbuild/`, `Mac/`, `Android/` | platform integration work |
| `editors/` | editor integrations |
| `Brand/` | canonical Noqeri brand assets |

## License

Noqeri is distributed under **GNU GPL v3 only (GPL-3.0-only)**. See [`LICENSE`](LICENSE).

Made by [**Noethric**](https://noethric.xyz).
