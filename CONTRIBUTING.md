# Contributing to Ric

Ric accepts focused changes that preserve the language contract and keep Linux, Windows and macOS bootstrap builds healthy.

## Development flow

1. Build with CMake (`scripts/build.sh` or `scripts/build.ps1`).
2. Add or update `.ric` tests for language behavior.
3. Run `ric check`, `ric test`, and `ric doctor`.
4. Run the platform-appropriate release script before opening a pull request.
5. Update `Doc/` when behavior or user-facing syntax changes.

## Repository ownership

Compiler implementation belongs in `compiler/bootstrap/` until a self-hosted Ric compiler replaces it. Public API changes belong in `Include/ric/`. Grammar changes must update `Grammar/ric.grammar.md`, tests and the language reference together.

Do not add empty placeholder modules. A new file should implement or document a real responsibility.

## Compatibility

`.ric`, `project.ric`, `ric.lock` and the `ric` command are the public names. New user-facing references to the previous language name are not accepted.
