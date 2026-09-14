# Ownership, compile-time execution and concurrency

This document describes the first compiler-enforced ownership and deterministic compile-time execution model shipped by the native/bootstrap Noqeri compiler. It is deliberately smaller than Rust's model: the goal is strong default safety with a compact language surface, then expansion only where tests demonstrate a need.

## Ownership v1

Noqeri classifies ordinary scalar values as copyable. `bool`, fixed-width integers, `int`, `float`, `string`, raw pointers and slices can be copied without invalidating the source binding.

Fixed arrays, records and other non-primitive value types are owned. Passing an owned value to a by-value parameter, assigning it into another owned binding, embedding it in another aggregate or returning it transfers ownership. The old binding may not be read again unless it is assigned a fresh value.

```text
function consume(values: [int; 2]): int { return values[0] }

let values: [int; 2] = [4, 8]
consume(values)          // ownership moves
print(values[0])        // compiler error: NQR-O5001
```

`&value` creates a tracked shared borrow. An owned value cannot be moved or mutated while a tracked borrow remains live. Existing lifetime analysis continues to reject references that outlive the storage they point to, including aggregate field escapes and interprocedural escapes.

```text
let values: [int; 2] = [4, 8]
let view: *[int; 2] = &values
consume(values)          // compiler error: NQR-O5002
```

Raw-pointer dereference/indexing and dangerous casts remain behind explicit `unsafe { ... }` blocks. Runtime bounds, null and optional checked-overflow protections remain separate layers; ownership does not weaken them.

### Concurrency boundary

Known spawn/task-transfer calls reject a borrow of a local binding crossing the task boundary (`NQR-O5004`). Owned values can instead be transferred into the spawned work. The compiler currently recognizes the built-in/runtime naming forms used by Noqeri's task and thread APIs; a future compiler-owned task syntax can replace the name-based bridge without weakening this rule.

Ownership v1 does **not** yet claim Rust-equivalent lifetime generics, mutable-borrow exclusivity syntax, `Send`/`Sync` traits or affine capabilities. Those should be added only together with type-system and self-hosted-compiler parity tests.

## First-class deterministic compile-time execution

`comptime(expression)` is a compiler-owned operation. It is parsed with ordinary call syntax, evaluated before type checking/lowering and replaced by the resulting literal, so a successful `comptime` call does not survive into NIR or runtime code.

```text
function fib(n: int): int {
    let a: int = 0
    let b: int = 1
    let i: int = 0
    while i < n {
        let next: int = a + b
        a = b
        b = next
        i = i + 1
    }
    return a
}

const answer: int = comptime(fib(10))
comptime_assert(answer == 55)
```

The CTFE engine supports deterministic pure Noqeri functions, local `let`/`const` values, arithmetic, comparisons, boolean logic, casts, branches, loops and returns. Integer arithmetic is overflow-checked while evaluating.

Compile-time execution is intentionally capability-free: host/ABI calls, pointer dereference, raw pointer access and runtime-only state are rejected. Evaluation is bounded to 100,000 interpreter steps and 128 call frames so a compiler invocation cannot be trapped indefinitely by accidental compile-time code.

`comptime_assert(expr)` requires a compile-time boolean and turns `false` into a compiler diagnostic.

## Structured-concurrency foundations

`std/concurrency` complements the existing task, channel, thread, sync, atomic, race-detector and future modules with host-neutral structured-concurrency primitives:

- cancellation tokens;
- countdown latches;
- task-group active/completed/failed/cancelled accounting;
- first-error capture;
- bounded backpressure gates;
- cancellation-aware bounded spinning.

`std/scheduler` now provides explicit scheduling policy, state transitions, deadlines, priority ordering, fairness/yield decisions and bounded contention budgets. These modules define policy/state; actual OS thread/task creation remains a runtime capability.

The ownership checker and concurrency library are designed together: borrowing local storage across a spawned task boundary is rejected, while moving owned state is the safe transfer mechanism.

## Fuzzing and compiler tooling

`Tools/fuzz/fuzz-all.mjs` now supports deterministic seeds, persistent corpora, replay, delta minimization, stable failure deduplication, machine-readable reports and optional native-vs-stage1 differential checking. Failures are saved under `build/fuzz-crashes`; reusable corpus inputs live under `build/fuzz-corpus`.

Examples:

```text
node Tools/fuzz/fuzz-all.mjs --cases=2000 --seed=12345
node Tools/fuzz/fuzz-all.mjs --replay=build/fuzz-crashes/parser-....nqr --target=check
NOQERI_STAGE1=./build/noqeri-stage1 node Tools/fuzz/fuzz-all.mjs --cases=500
```

A fuzz run treats compiler crashes, unexpected exit codes, timeouts and launch failures as distinct failure classes. The minimizer keeps reducing an input while the same command remains failing, producing a smaller regression artifact suitable for a permanent test.

## Bootstrap relationship

Ownership and CTFE are compiled into the C++ source-bootstrap/native compiler. A clean machine can now construct that seed from this repository automatically when no Noqeri executable exists. The C++ source remains the stage-0 trust root until stage1→stage2 reproduction/equivalence is demonstrated; see `Compiler/selfhost/BOOTSTRAP.md`.
