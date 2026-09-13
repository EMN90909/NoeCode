# Security response process

This document is operational policy, not a claim that Noqeri has completed an external security audit.

## Intake and coordinated disclosure

Security reports should use the contact in `SECURITY.md` / `.well-known/security.txt` and avoid public issues until a fix or disclosure date is agreed. A maintainer acknowledges a credible report, assigns a private tracking identifier, records affected versions/components, and establishes an embargoed reproduction when appropriate.

## Triage

Classify impact separately for compiler, runtime, package manager/registry, standard library, build/release chain, and hosted services. Determine whether untrusted source, package metadata, network input, database input, object files, or generated artifacts can trigger the issue. Security-sensitive parser crashes and sanitizer findings are release blockers until triaged.

## CVE/advisory handling

For vulnerabilities affecting released users, prepare an advisory containing affected/fixed versions, severity rationale, prerequisites, workarounds, credit, and upgrade guidance. Request a CVE when appropriate and publish registry advisory metadata so `noqeri audit` can consume it. Do not reserve or publish a CVE merely for marketing.

## Supported versions

Until a formal LTS line is announced, the security support target is the latest stable release plus any explicitly documented supported predecessor. Experimental/preview package versions have no implied long-term support. A release must publish its support status rather than leaving users to infer it.

## Fix and validation

A security fix needs a regression test that fails before the fix and passes after it when disclosure safety permits. Relevant fuzz target(s), memory/race/overflow checks, package integrity tests, and reproducible-release checks should be rerun. Secret test vectors may remain private until disclosure.

## Release evidence

Official release artifacts should include platform/architecture name, SHA-256 checksums, a signature, an SBOM, source revision, compiler provenance, and the result of reproducibility/security gates. Windows, Linux, macOS and supported ARM builds should follow the same manifest format.

## Postmortem

After coordinated disclosure, record root cause, why existing tests missed it, which fuzz/property corpus was added, and whether language/library complexity contributed. Prefer removing unsafe ambiguity to adding another user-visible feature.
