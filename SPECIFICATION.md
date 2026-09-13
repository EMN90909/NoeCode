# Noqeri Language Specification — 1.x Draft Contract

This document is normative for the 1.x language family unless a section is explicitly marked provisional.

## Compatibility

Programs valid under Noqeri 1.x are intended to continue compiling under later Noqeri 1.x releases, except for documented security corrections. New keywords or semantic restrictions must not silently reinterpret previously valid programs.

## Lexical rules

Source is UTF-8. Identifiers begin with an ASCII letter or underscore and continue with letters, digits or underscore. `//` and `/* ... */` are comments. String literals use double quotes with standard escaped backslash, quote, newline, carriage return, tab and NUL forms.

## Evaluation order

Expressions evaluate left-to-right. Function arguments evaluate left-to-right before the call. Short-circuit `&&` and `||` evaluate their right operand only when required.

## Integers

`int` is a signed implementation-stable 64-bit integer for Noqeri 1.x. Fixed-width signed and unsigned integer types preserve their declared widths. Narrowing and signedness-changing conversions require explicit casts. Overflow behavior for checked safe arithmetic is an error contract; optimizer transformations must preserve observable behavior.

## Memory and unsafe code

Safe code may use values, records, arrays, slices and safe library abstractions without entering `unsafe`. Raw pointer dereference, volatile access, arbitrary pointer/integer reinterpretation, inline assembly, machine intrinsics, unrestricted ABI/FFI calls and calls to raw `extern` functions require a lexical `unsafe { ... }` block. Unsafe code remains type checked.

## Concurrency

Noqeri uses a data-race-free concurrency contract. Tasks and threads may communicate through channels or synchronized shared memory. Mutex, RW-lock, channel and atomic operations establish synchronization edges. Conflicting unsynchronized access from multiple threads is a data race and outside the safe-language guarantee. Structured task scopes own their child tasks until joined, cancelled or explicitly detached.

## Errors

Compiler diagnostics have stable machine-readable codes. Error messages should identify the source location, actual condition, expected condition and, where practical, a corrective suggestion. Runtime failures must not be reported as successful exits.

## Imports and packages

Imports identify Noqeri source modules. Package resolution must be deterministic for a given manifest and lockfile. Locked dependencies are content-addressed with SHA-256 or stronger hashes and must be verified transitively before execution or build.

## ABI

The public embedding ABI is versioned independently from source compatibility. ABI consumers must validate ABI version and structure size before invoking services.

## Database formats

`.nqd` is the concise native NoqeriDB language. `.sql` is the standards-oriented query interface. Both operate against the same transactional storage engine and must preserve transaction isolation and recovery guarantees.

## Undefined behavior

The safe language aims to contain no intentional undefined behavior. Operations whose safety cannot be proven must be isolated behind `unsafe`; unsafe code may violate memory safety if its documented preconditions are not upheld.

## Stabilization rule

After Noqeri 1.0 syntax is declared stable, basic syntax will not be changed incompatibly inside the 1.x series. Extensions should prefer additive grammar and library APIs.
