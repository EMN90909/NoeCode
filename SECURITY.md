# Security policy

Noqeri treats compiler, runtime, package-manager, registry and release-chain defects as security-sensitive when they can cross a trust boundary or violate the language safety contract.

## Reporting a vulnerability

Prefer GitHub private vulnerability reporting for this repository when it is available. Do not open a public issue for an unpatched vulnerability. If private reporting is unavailable, contact the maintainer through the security contact published in the repository metadata and state only that you need a private security channel; do not include exploit details in a public message.

A useful report includes:

- affected Noqeri version/commit and platform;
- affected command, package or runtime provider;
- minimal `.nqr`, `.nqd`, SQL, package or binary input when practical;
- observed and expected behavior;
- security impact and required attacker capabilities;
- whether the issue is already public;
- proposed mitigation, if known.

## Response process

1. **Triage.** Confirm receipt, reproduce when possible, assign severity and affected supported versions.
2. **Containment.** Prepare the smallest safe fix and a regression test. Registry packages may be yanked while remaining identifiable and verifiable.
3. **Coordination.** Agree on a disclosure date with the reporter when advance coordination is useful. Downstream maintainers may receive fixes privately when needed to avoid exposing users before patches are ready.
4. **Release.** Publish patched supported releases, checksums/signatures/SBOM, advisory information and upgrade instructions.
5. **CVE.** Request or publish a CVE when the issue has ecosystem-visible security impact and meets CVE assignment criteria. The advisory must map the CVE to affected/fixed Noqeri and package versions.
6. **Postmortem.** Add a regression test, fuzz seed or conformance case so the class of failure is harder to reintroduce.

## Severity priorities

Highest priority includes:

- safe-code memory-safety failures or sandbox escapes;
- compiler miscompilation that defeats documented safety checks;
- malicious-source, `.nqd`, SQL, object, network or package inputs that cause exploitable memory corruption;
- package/path traversal, integrity/signature bypass or dependency-confusion failures;
- TLS/certificate validation bypass in official providers;
- race-detector/sanitizer blind spots that permit a documented safe-code guarantee to be bypassed;
- database corruption/recovery flaws that expose uncommitted or unauthenticated data;
- release signing, build provenance or registry compromise.

Ordinary compiler crashes from malformed input are bugs and fuzz targets; they become security issues when they cross a meaningful trust boundary, enable memory corruption/resource exhaustion, or affect a service that processes untrusted input.

## Supported versions

Noqeri is pre-1.0. Until 1.0, security fixes target the current supported development line and the most recent published preview where a patch can be produced safely. Once 1.0 is released, the project will publish an explicit supported-version table in release notes and this file. A stable major line must receive security fixes for the documented support window; unsupported versions may receive advisories without a backport.

Official registry packages use the same quality/support vocabulary: `experimental`, `preview`, `stable`, and `core`. Experimental/provider-stub packages do not inherit the same support promise as `core` packages.

## Advisories and audit data

Machine-readable Noqeri/package advisories live under `security/advisories.json`. `noqeri audit` is expected to check exact locked package versions against that database in addition to validating lockfile integrity. Public advisories should include affected ranges, fixed versions, severity, identifiers and mitigation.

## Disclosure principles

Noqeri favors coordinated disclosure, narrowly scoped embargoes and prompt patch availability. Security fixes must not silently weaken the language's safety, package-integrity or compatibility guarantees. Security-driven compatibility exceptions must be documented and regression-tested as required by `Doc/COMPATIBILITY.md`.
