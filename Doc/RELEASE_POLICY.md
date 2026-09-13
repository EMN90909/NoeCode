# Noqeri release policy

An official Noqeri release is a reproducible set of signed artifacts, not merely a Git tag.

## Required artifacts

A stable release must publish, for every platform marked supported by that release:

- Windows x86-64 archive/installer;
- Linux x86-64 archive/binary;
- macOS x86-64 and/or ARM64 archive as documented;
- Linux ARM64 when the backend/runtime is marked supported;
- `SHA256SUMS` covering every downloadable artifact;
- an Ed25519 signature envelope for `SHA256SUMS`;
- `release-manifest.json` with source commit and artifact metadata;
- SPDX SBOM (`sbom.spdx.json`);
- release notes identifying compatibility/security changes.

A platform whose backend/provider tests do not pass is omitted or labeled preview; an empty placeholder archive does not satisfy the matrix.

## Reproducibility

Release builds set a documented `SOURCE_DATE_EPOCH`, locale and timezone. `scripts/reproducible-build.mjs` builds the same source/dependencies twice into clean directories and compares the complete artifact trees. Stable releases require bit-for-bit identity where supported; unavoidable platform signing/notarization metadata must be separated from the reproducible unsigned payload and documented.

The exact compiler source commit, language edition, lockfiles, dependency hashes and build command are release inputs.

## Signing

Release signing uses an Ed25519 release key distinct from ordinary package-publisher keys. Public key fingerprints are published through at least two independent project-controlled locations. Private release keys must not be stored in the repository or passed through command-line arguments. `scripts/release-artifacts.mjs` accepts signing material through a protected environment/secret channel.

Key rotation publishes old/new fingerprints and a transition statement. A compromised key is revoked immediately and affected artifacts are reissued from a reviewed commit.

## SBOM

The release SBOM lists the Noqeri release plus packaged files/dependencies using SPDX 2.3 or a later explicitly supported format. Platform installers may add a platform-specific SBOM in addition to the common manifest.

## Performance gate

Stable runtime/compiler releases run the canonical `Benchmarks/suite.json` workloads and compare Noqeri medians to an accepted baseline with `scripts/perf-gate.mjs`. Regressions beyond the workload threshold block release until accepted with a written rationale and updated baseline.

Cross-language benchmark results are informational. Release notes must not claim Noqeri is faster than Rust, Zig, C, C++ or Go unless the published benchmark result and methodology support that exact statement.

## Security gate

Release candidates run:

- conformance and differential suites;
- deterministic fuzz smoke and extended fuzz campaign;
- memory/overflow runtime modes;
- race checks where concurrent execution exists;
- lock/integrity/advisory audit;
- native ASan/UBSan build on supported compilers;
- TSan build separately where supported;
- reproducibility verification;
- artifact checksum/SBOM/signing generation.

## No mandatory GitHub Actions dependency

These are local/reproducible commands. CI may automate them later, but release correctness must not depend on paid or proprietary CI execution. A maintainer or downstream builder must be able to reproduce the gates locally.
