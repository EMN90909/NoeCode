# Noqeri Language Specification — 2026 Edition

Status: normative foundation for the 2026 edition. Where this document conflicts with examples or prose documentation, this document is intended to win once the corresponding conformance test exists. Features marked **provisional** are not part of a future 1.x compatibility promise until promoted.

## 1. Source and lexical rules

Noqeri source is UTF-8 and uses the `.nqr` extension. ASCII letters, digits and `_` form identifiers; an identifier may not begin with a digit. Whitespace separates tokens where concatenation would otherwise change tokenization. `//` starts a line comment and `/* ... */` a block comment. Comments do not affect program behavior.

String literals use double quotes. `\\`, `\"`, `\n`, `\r`, `\t` and `\0` are defined escapes. An unterminated string or block comment is a compile error.

Keywords include `module`, `import`, `record`, `let`, `const`, `function`, `extern`, `export`, `if`, `else`, `while`, `return`, `throw`, `try`, `as`, `volatile`, and `unsafe`.

## 2. Modules and imports

A source file may declare one module. File imports use a quoted path. Registry imports use `import package "namespace/name"` and must correspond to an exact dependency in `project.nqr` and `noqeri.lock` before execution or compilation.

Import resolution must be deterministic. The same project, lockfile, compiler edition and target must resolve the same package versions and package integrity hashes.

## 3. Primitive types

The fixed-width integer types are `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, and `u64`. `isize` and `usize` are pointer-width signed and unsigned integers. `int` is the language's ordinary signed integer type and is currently 64-bit in the 2026 edition. `bool`, `float`, `string`, `void`, pointers, fixed arrays and slices are built-in types.

Implicit integer conversion is permitted only when every value representable by the source type is representable by the destination type. Narrowing and signedness-changing conversions require `as`. Integer literals may initialize narrower integer types when the literal is representable.

**Provisional:** release-mode overflow policy is not frozen for 1.0. Until it is, programs must not rely on overflow wrapping unless they use an explicitly wrapping standard-library operation.

## 4. Evaluation order

Operands and function arguments are evaluated left-to-right. `&&` and `||` short-circuit and do not evaluate the right operand when the result is already determined. Assignment evaluates its right-hand side before storing the resulting value into the left-hand location.

Observable side effects must preserve this order in the interpreter, native backend and `.nqo`/web backend.

## 5. Functions and errors

Functions declare parameters and an optional result type. `return` exits the current function. `throw` and unary `try` are the lightweight status-propagation mechanism currently implemented by the compiler and require compatible integer status returns.

`extern function` declares a host/ABI capability. Direct unrestricted host calls are not ordinary safe operations; standard-library packages should wrap them in narrow safe APIs.

## 6. Records, arrays and slices

Records have declaration-order fields with target ABI alignment. Fixed arrays own a compile-time number of elements. Slices are a data pointer plus length view and do not own their referenced storage.

Known fixed-array indices outside the declared bound are compile errors. Returning a pointer/slice borrowed from function-local storage is invalid. A slice constructed from a raw pointer and length crosses the unsafe boundary.

## 7. Pointers, volatile memory and `unsafe`

Safe Noqeri code must not perform operations whose correctness depends on unchecked memory or external ABI invariants.

The explicit syntax is:

```nqr
unsafe {
    // narrowly scoped operation
}
```

The following operations require an unsafe context when the compiler can identify them:

- raw pointer dereference and raw pointer field access;
- volatile raw-memory access;
- creation of slices from raw pointer + length;
- casts that create/reinterpret raw pointers;
- inline assembly and architecture/compiler intrinsics;
- unrestricted `host(...)` / `abi(...)` capabilities;
- FFI operations declared unsafe by their provider contract.

`unsafe` does not disable type checking, bounds checks that are otherwise knowable, lifetime diagnostics, or syntax checking. It states that the programmer/provider accepts additional proof obligations. Unsafe blocks should therefore be as small as practical.

## 8. Concurrency and memory model

Noqeri's concurrency model consists of tasks, OS threads where explicitly requested, channels, mutexes, reader/writer locks and atomics. Structured task scopes own child tasks and propagate cancellation.

A program has a data race when two concurrent executions access the same memory location, at least one access is a write, and the accesses are not ordered by synchronization. Data races are invalid program behavior.

Channel send/receive pairs, successful lock/unlock handoff, task join, and sequentially consistent atomic operations establish synchronization ordering. The initial atomic standard library is sequentially consistent by default; weaker orderings are provisional until the memory model and optimizer have conformance tests for them.

Cancellation is cooperative: requesting cancellation makes the state observable; code must reach a cancellation-aware operation or explicitly check cancellation. Timeouts are expressed in non-negative milliseconds unless a specific API states otherwise.

## 9. Files, networking and external capabilities

Filesystem, networking, process, database and cryptographic providers are capabilities. Application-facing standard APIs are safe wrappers; OS handles, sockets, native library calls, memory mapping and similar primitives remain behind unsafe host boundaries.

HTTP implementations must enforce bounded headers/body sizes and timeouts at the provider layer. TLS certificate verification must default to enabled.

## 10. Database semantics

`.nqd` is the native NoqeriDB schema/query language. `.sql` is the interoperability query language. SQL values must be passed as parameters rather than concatenated into query text.

A committed transaction must be durable before success is reported. Recovery must either expose the complete committed transaction or the state before it; partially committed user-visible state is invalid. Corrupt authenticated pages/WAL records must fail closed.

## 11. Package and build reproducibility

Registry versions are immutable once published. Yanked versions remain identifiable but are not selected for new resolutions. `noqeri.lock` records exact versions, canonical entry paths and `sha256:<64 lowercase hex>` package digests. Cached package content is re-hashed before use.

A frozen build must reject a project whose declared dependencies disagree with the lockfile.

## 12. ABI

The supported target determines pointer width, calling convention and object format. Record layout follows the target ABI unless a future explicit representation attribute says otherwise. ABI-facing APIs are compatibility-sensitive and must not change silently within a stable 1.x line.

## 13. Undefined and invalid behavior

The language aims to minimize undefined behavior. Safe-code violations should be compile errors or defined runtime failures. Unsafe code may become invalid when it violates pointer validity, alignment, lifetime, synchronization, ABI, or provider preconditions stated by this specification/API documentation.

## 14. Compatibility

The 2026 edition is pre-1.0 and may evolve through documented edition changes. Once Noqeri 1.0 is declared, the compatibility policy in `Doc/COMPATIBILITY.md` governs the 1.x line.

## 15. Conformance

A language rule is considered release-gated when it has a conformance test. The canonical suite contains valid programs, invalid programs with required diagnostic/status classes, edge cases and differential programs whose observable results must agree across execution backends.
