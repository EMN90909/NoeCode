# Noe 1.0 production-track readiness

NoeCode is being moved onto a `1.0.0-production-track` baseline for the supported bootstrap subset.

This means the repository is structured, documented, licensed, testable, and executable as a real compiler project. It does **not** mean every future Noe language feature is complete.

## Production-track guarantees for the supported subset

The supported subset is expected to remain buildable and testable from a clean clone:

```sh
sh scripts/build.sh
sh scripts/test.sh
sh scripts/release_check.sh
```

The release check verifies:

- compiler build succeeds;
- interpreter execution matches expected output;
- native Linux x86-64 build succeeds;
- generated executable output matches expected output;
- type errors are rejected;
- const reassignment is rejected;
- formatter, test runner, manifest, lockfile, project creation, and LSP diagnostics are exercised;
- public documentation, license, legal notice, security policy, and contribution guide exist.

## Supported 1.0 production-track language subset

- top-level statements;
- `let` and `const` declarations;
- primitive `int`, `float`, `bool`, `string`, `null`, and `void` type model;
- type annotations and type inference for primitives;
- functions with typed parameters and return types;
- `if` / `else`;
- `while` loops;
- assignments to mutable `let` values;
- compile-time rejection of assignment to `const` values;
- arithmetic, comparison, equality, and boolean operators;
- calls to user functions and builtin `print`;
- NIR lowering, optimization, interpretation, and Linux x86-64 native build for the supported native subset.

## Non-goals for this specific baseline

The complete long-term Noe language still needs later milestones for modules, packages from remote registries, arrays, records, classes, generics, interfaces, async/await, C ABI FFI, standard-library expansion, multi-platform native backends, freestanding/system profile, visual frontend, and self-hosting.

NoeCode must be honest about those boundaries. Unsupported native operations must fail with diagnostics rather than silently emitting incorrect machine code.

## Release standard

A future full Noe 1.0 release should require:

- stable language specification;
- stable standard library API;
- multi-file/module compilation;
- robust error recovery and structured diagnostics;
- package resolution with integrity checks;
- editor integration tests;
- cross-platform release artifacts;
- security review of the compiler input surface;
- native backend conformance tests;
- clear compatibility policy.
