# Noqeri architecture

Noqeri uses a **Noqeri-first implementation rule**: reusable language-facing behavior should be written in `.nqr` when the language can express it. C++ remains the bootstrap layer for the compiler, parser, code generators, reference interpreter, ABI bridge, and host/tool adapters that currently require filesystem, process, or platform access Noqeri cannot yet provide directly.

The compilation path is:

`Noqeri source -> lexer/parser -> checked AST -> generics/lifetime analysis -> typed NIR -> optimizer -> interpreter/native/web backend`

`Include/noqeri/noqeri.hpp` is the public bootstrap API contract. The implementation retains one internal compatibility namespace/include name from the earliest bootstrap; those names are not part of the `.nqr` language surface or installed API and can be removed at a deliberate compiler-ABI break.

## Noqeri-owned implementation

The repository no longer uses Markdown placeholders as substitutes for these implementation areas:

- `Modules/builtin.nqr` owns executable helpers built on the language's actual built-ins (`len`, slice values and typed integer operations).
- `Modules/native.nqr` owns source-level native-module compatibility/version/status rules.
- `Objects/value.nqr` owns the source-level ABI-compatible value-tag/header contract.
- `Objects/text.nqr` owns byte-string comparison and prefix semantics.
- `Database/noqeridb.nqr` owns the first executable database-core algorithms: fixed-capacity key lookup, insert, update, delete and read operations.
- `Tools/security/policy.nqr` owns reusable range/index/full-scan validation helpers.
- `Lib/test/assert.nqr` owns reusable Noqeri test assertions.

`tests/noqeri_owned_components.nqr` imports these areas together and executes them through the real compiler/runtime. CTest also runs this integration fixture explicitly.

## Bootstrap boundary

The following C++ remains intentional bootstrap/host infrastructure rather than application library code:

- `Parser/` and the main `Compiler/` sources: source frontend, checking, NIR, optimization and code generation.
- `Runtime/`: the reference NIR interpreter and ABI bridge used to bootstrap and test Noqeri programs.
- `Programs/`: CLI/LSP/formatter/package/test host adapters.
- `Compiler/bootstrap/noqeridb_host.cpp`: compatibility adapter for the existing `.nqd` grammar and persistent `.nqdb` file format. Database algorithms are being moved into `Database/noqeridb.nqr`, but deleting this adapter today would remove working typed text/real/bool tables and persistence before equivalent typed filesystem capabilities exist in Noqeri.
- `Compiler/bootstrap/security_audit_host.cpp`: repository/package-lock scanner. Reusable security policy belongs in Noqeri; the scanner still needs host filesystem traversal.

The `Database/`, `Modules/`, `Objects/`, `Lib/`, and `Tools/security/` implementation directories are Noqeri-owned; their C++ environment bridges, when still necessary, are isolated under `Compiler/bootstrap/`.

That distinction is deliberate: **do not rewrite a working host boundary in Noqeri by replacing it with an unimplemented filesystem/network primitive.** Move logic inward as the language gains the capability to execute it, while keeping interoperability at a narrow bootstrap edge.

## Ownership rule

When adding functionality, use this order:

1. Implement reusable algorithm/policy/library behavior in `.nqr`.
2. Expose the smallest necessary host capability through the ABI when the language cannot reach the environment itself.
3. Keep C/C++ inside compiler/bootstrap/reference-host code rather than growing new C++ application libraries.
4. Add a `.nqr` integration test so a source file is executable behavior, not another placeholder.

The Unix/Windows repository verifiers and `noqeri doctor` enforce this boundary by rejecting C/C++ source inside the Noqeri-owned implementation directories and by rejecting the removed Markdown placeholder paths.

This is the path toward progressive self-hosting without pretending the bootstrap is already self-hosted.
