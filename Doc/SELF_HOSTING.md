# Noqeri self-hosting model

Noqeri uses staged bootstrap with a strict provenance boundary.

## Stage 0 — archival seed

The historical C++17 compiler remains only on `bootstrap/cpp17-stage0`. It is provenance: a trusted seed capable of creating the first Noqeri compiler object on a machine that has no Noqeri compiler yet. It is not the intended implementation language of `main` and the normal compiler source does not grow new C++ features.

## Stage 1 — Noqeri compiler source

`Compiler/selfhost/` contains Noqeri-owned modules for lexical scanning, AST/type tags, structural parser policy, semantic conversion rules, NIR opcodes, optimisation policy, package imports, target/ABI selection, PE/COFF/ELF/Mach-O format policy, backend/link policy, formatter policy and LSP diagnostics. The normal bootstrap entry point is `Compiler/selfhost/bootstrap.nqr`.

## Host boundary

Filesystem, process, terminal, network and executable-page services may be supplied by a portable host adapter. Compiler language semantics and code-generation policy belong in `.nqr`. A host adapter must not silently reimplement the frontend in JavaScript or C++.

## Stage-1 → stage-2 completion gate

Full self-hosting is reached only when all are true:

1. stage 0 builds stage 1;
2. stage 1 compiles the complete compiler source into stage 2;
3. stage 2 rebuilds the same source;
4. stage-1/stage-2 outputs are byte-identical where reproducibility permits, or pass semantic-equivalence checks where platform metadata differs;
5. parser, type checker, NIR, optimiser, native/object writers, formatter, LSP and package-manager conformance suites pass without calling the archival C++ seed;
6. the normal release process no longer needs CMake/C++.

The repository must not claim this gate passed until an actual Noqeri compiler binary executes these steps. Source ownership and architecture can move ahead of the bootstrap proof; documentation cannot substitute for that proof.

## Local verification

GitHub Actions are not required. Release scripts should run locally or on self-managed target machines and retain equivalence hashes, conformance results and native-object validation reports as release evidence.
