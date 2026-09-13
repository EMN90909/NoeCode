# Production readiness

Noqeri is being hardened toward production use with evidence-based gates rather than version-number claims.

## Implemented source surfaces

- Noqeri-owned stage-1 bootstrap source under `Compiler/selfhost/`.
- Noqeri-owned AST, parser-policy, semantic, NIR, optimiser, target/ABI, backend/link, formatter and LSP modules.
- Package manager with SemVer selection, SHA-256 cache verification, offline mode, dependency-depth/conflict limits, traversal rejection and lockfiles.
- NoqeriDB modules for limits, security, indexes, transactions, WAL/recovery policy, migrations, planning and storage capabilities.
- Expanded standard math and a broader package/framework ecosystem.
- Target descriptions for Windows/Linux/macOS on x86-64 and ARM64.

## Gates that still decide a production release

A source file existing is not equivalent to a proven compiler/database/runtime implementation. Before a stable production claim, local release evidence must include:

- stage1 → stage2 → stage3 compiler rebuild and semantic-equivalence comparison;
- positive/negative language conformance and fuzzing;
- independent validation of emitted PE/COFF, ELF and Mach-O objects;
- executable/link tests on Windows x64/ARM64, Linux x64/ARM64, macOS x64/ARM64;
- cross-compilation fixtures, static/shared libraries and debug-symbol validation;
- NoqeriDB crash/fault injection, WAL recovery, corruption, transaction atomicity/isolation and index/constraint tests;
- package-manager tamper/offline/dependency-conflict tests;
- performance and memory regression suites;
- reproducible signed release artifacts.

The project intentionally does not use GitHub Actions as the only source of truth. These gates can be run locally and on self-managed machines without consuming GitHub-hosted Actions minutes.
