# Noqeri 1.x compatibility promise

Noqeri 1.x is intended to be a stable language line.

> A program that is valid under a released Noqeri 1.x language specification will continue to compile under later Noqeri 1.x releases, except where a documented security correction is required to prevent memory unsafety, data corruption, or a supply-chain vulnerability.

## What is covered

The promise covers documented source syntax, type-system behavior, standard scalar widths, module/import syntax, error propagation, the public standard-library compatibility surface, project manifest syntax, lockfile reading, and the stable Noqeri ABI documented for the same target family.

Additions may be made in 1.x when they do not change the meaning of existing valid programs. New warnings may be introduced. Diagnostics and optimizer output may improve without being source-compatibility breaks.

## Security exceptions

A security exception must be narrow, documented in the changelog, include a migration path where technically possible, and be treated as a release-blocking compatibility event. The `unsafe` boundary introduced during the 1.x stabilization period is such a hardening mechanism: low-level operations are made explicit rather than removed.

## Syntax freeze

Once Noqeri 1.0 is declared stable, basic syntax is frozen for the 1.x line. Proposals that merely provide another spelling for an existing operation should normally be rejected. New syntax requires a specification change, conformance tests, formatter support, LSP support, diagnostics, and a compatibility analysis before release.

## Packages

Published package versions are immutable. A bad release is yanked; it is never replaced in place. Lockfiles continue to identify the exact package version and content hash. A future transparency service may add verification evidence without changing package source semantics.
