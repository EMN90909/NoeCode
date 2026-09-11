# Security policy

## Supported branch

Security fixes target the `main` branch while Ric is on the 1.0 production track.

## Reporting

Please avoid publishing exploit details in a public issue before maintainers have had a reasonable opportunity to investigate. Use GitHub's private vulnerability reporting feature when it is enabled for this repository.

Include the affected command, platform, compiler build information, a minimal `.ric` reproducer and the security impact.

## Scope

High-priority reports include compiler memory-safety failures, malicious-source crashes with security impact, native-backend code-generation issues that can escape intended semantics, package/path traversal, unsafe LSP input handling and CI/release-chain compromise.
