# Security Policy

NoeCode is an experimental compiler/toolchain bootstrap. Do not use it yet for security-critical, safety-critical, medical, financial, aviation, industrial-control, or production isolation workloads.

## Reporting security issues

Please report security-sensitive issues privately to the repository owner instead of opening a public issue with exploit details.

## Current support status

Only the current `main` branch is maintained during bootstrap development. There are no long-term support releases yet.

## Scope

Security reports may include:

- incorrect code generation;
- crashes on malformed `.noe` input;
- unsafe file writes from CLI commands;
- path traversal in project/package tooling;
- generated binary behavior that differs from checked Noe semantics.

The project will add a fuller security process when Noe reaches stable release status.
