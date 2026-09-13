# Noqeri Compatibility Promise

Noqeri is currently pre-1.0. The 2026 edition may still make documented breaking changes while the self-host compiler reaches parity and the language specification closes provisional areas.

## Noqeri 1.x promise

When Noqeri 1.0 is released, the project will adopt this promise:

> Programs that are valid Noqeri 1.x programs will continue to compile and preserve their specified observable behavior under later Noqeri 1.x compilers on supported targets, except where a narrowly documented security correction is required.

This applies to language syntax, type checking, evaluation order, standard-library stable APIs, package manifest/lock formats marked stable, and published ABI contracts.

## What may change in 1.x

Changes are allowed when they are backward compatible, including new APIs, new diagnostics, optimizer improvements, new targets, additional syntax that does not change existing parsing, and performance improvements that preserve specified observable behavior.

Security exceptions must be rare, documented in release notes, covered by a regression test, and use the smallest breaking scope practical.

## What must wait for a new major version or edition

After 1.0, existing basic syntax must not be repurposed; valid constructs must not silently change meaning; stable std APIs must not be removed; integer/evaluation/memory semantics must not be changed incompatibly; and package integrity records must not be weakened.

A new edition may introduce opt-in language changes while retaining the ability to build older stable editions.

## Release gate

The 1.0 compatibility promise must not be activated until:

1. the normative specification has no unresolved core semantic placeholders;
2. self-host compiler parity is demonstrated against the trusted reference;
3. the conformance suite covers stable syntax/type/evaluation/memory rules;
4. differential tests pass across supported interpreter/native/web backends;
5. package lock/integrity behavior is reproducible;
6. standard-library stable/experimental API labels are published.

Until that point, release notes must identify breaking changes explicitly rather than implying 1.x stability early.
