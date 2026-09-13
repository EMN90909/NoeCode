# Noqeri self-hosting model

Noqeri uses a conventional staged bootstrap. A compiler cannot compile its own source on a machine that has never had any compiler binary, so the project distinguishes **provenance** from the **normal build**.

## Stage 0 — trusted seed

The final C++17 bootstrap source before the migration is preserved at branch `bootstrap/cpp17-stage0`, commit `82101a29de8ef75054e96bc13552b60f787dbc2e`.

Stage 0 exists only to create the first stage-1 portable object. It is not invoked after stage 1 has been produced and is not part of the normal compiler implementation path on `main`.

## Stage 1 — Noqeri-owned compiler kernel

`Compiler/selfhost/` is written in Noqeri and owns the first migrated compiler responsibilities:

- lexical classification and token boundaries;
- whitespace/comment/string scanning;
- delimiter and comment/string syntax validation;
- primitive integer type-width/sign rules;
- source token counting and deterministic source fingerprinting.

`./scripts/build.sh` and `scripts/build.ps1` use a trusted Noqeri stage-0 executable to compile `Compiler/selfhost/main.nqr` to `build/noqeri-stage1.nqo`. The checked-in Node launcher is a host adapter for portable filesystem/terminal access; language/compiler rules remain in Noqeri.

## Cross-platform boundary

The stage-1 artifact is `.nqo`, so the same compiled compiler kernel can run on Windows, Linux, macOS and any other platform that provides the portable Noqeri web-object host contract. The included reference launcher uses Node.js 20+.

Native code generation, linking, package resolution, NoqeriDB tooling, formatter and LSP are being moved subsystem-by-subsystem. Until a subsystem reaches parity, the preserved stage-0 seed is the reference implementation; it is intentionally not hidden behind the stage-1 launcher.

## Build locally

POSIX:

```sh
NOQERI_STAGE0=/path/to/trusted/noqeri ./scripts/build.sh
./build/noqeri selftest
./build/noqeri check examples/hello.nqr
```

PowerShell:

```powershell
$env:NOQERI_STAGE0 = 'C:\path\to\trusted\noqeri.exe'
.\scripts\build.ps1
.\build\noqeri.cmd selftest
```

No GitHub Actions are required. Local build/test is the canonical verification path during this migration.

## Completion criterion

The migration is complete when a stage-1 compiler can rebuild a byte-for-byte or semantically equivalent stage-2 compiler and all compiler commands pass the same conformance suite without calling the historical C++ seed. Until then, claims of a fully self-hosted compiler would be misleading.
