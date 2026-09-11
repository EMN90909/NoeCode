# Noe documentation

Welcome to the NoeCode documentation hub.

Noe is a standalone, statically typed, native-oriented general-purpose language. This repository contains the production-track bootstrap compiler and the currently supported 1.0 subset.

## Read first

- [`GETTING_STARTED.md`](GETTING_STARTED.md) — clone, build, run, and compile a `.noe` program.
- [`STATUS.md`](STATUS.md) — exact feature status and limitations.
- [`PRODUCTION_READINESS.md`](PRODUCTION_READINESS.md) — what the 1.0 production-track baseline means.
- [`PLATFORM_SUPPORT.md`](PLATFORM_SUPPORT.md) — Linux, Windows and macOS support matrix.
- [`CPYTHON_STRUCTURE_REVIEW.md`](CPYTHON_STRUCTURE_REVIEW.md) — how NoeCode was reorganized after reviewing CPython's mature repository layout.
- [`LANGUAGE_REFERENCE.md`](LANGUAGE_REFERENCE.md) — current syntax and semantics.
- [`TOOLCHAIN.md`](TOOLCHAIN.md) — compiler pipeline, CLI, diagnostics, formatter, tests, and LSP.
- [`NATIVE_BACKEND.md`](NATIVE_BACKEND.md) — custom Linux x86-64 backend details.
- [`PROJECTS.md`](PROJECTS.md) — `project.noe` and `noe.lock`.
- [`ROADMAP.md`](ROADMAP.md) — path from bootstrap to full Noe ecosystem.

## Repository areas

- `compiler/bootstrap/` — current C++17 bootstrap compiler.
- `include/noe/` — bootstrap compiler header/API surface.
- `grammar/` — grammar and syntax notes.
- `stdlib/std/` — seed Noe standard-library modules.
- `runtime/` — runtime design area.
- `platforms/` — platform-specific build/target notes.
- `tools/` — future developer tooling.

## Important honesty rule

The repository is production-track for the currently implemented bootstrap subset. It is not yet the final complete Noe language specification or complete Noe standard library.

Unsupported features are documented as future work instead of being hidden or implied.
