# Unsafe boundary and concurrency contract

Noqeri safe code is the default. Operations that can violate memory or ABI safety must be lexically contained by `unsafe { ... }`.

The compiler treats the following as unsafe operations:

- dereferencing raw pointers (`*ptr`)
- pointer/integer and unrelated pointer casts
- volatile pointer access
- inline `asm(...)` and machine `intrinsic(...)`
- direct `host(...)` / `abi(...)` service invocation
- calls to `extern` functions

Address-of (`&value`) and safe atomics are not, by themselves, unsafe. Standard-library wrappers may contain a small audited unsafe block and expose a safe API.

## Example

```noqeri
function read_register(ptr: *volatile u32): u32 {
    unsafe {
        return *ptr
    }
}
```

Unsafe blocks do not disable type checking, bounds checking for arrays/slices, or borrow/lifetime analysis. They only authorize operations that the compiler otherwise rejects with `NQR-T3060`.

## Concurrency memory model

Noqeri's concurrency contract is data-race-free by default:

- task spawn establishes a parent-to-child synchronization edge for work sequenced before spawn;
- successful task join establishes a child-to-parent edge for work after join;
- successful channel send/receive, mutex/RW-lock operations and atomic synchronization establish synchronization edges;
- sequentially-consistent atomics establish a total order for participating atomic operations;
- cancellation is cooperative and long-running tasks can poll `task_cancelled_current()`;
- structured task scopes must not return while child tasks remain unjoined unless they were explicitly detached;
- unsynchronized conflicting memory accesses from different threads are a data race and outside the safe-language contract.

The reference runtime backs tasks with C++17 worker threads, channels with bounded queues/condition variables, mutexes with timed mutexes, and RW locks with shared timed locks. This is deliberately a runtime implementation detail; ordinary Noqeri code uses the std APIs rather than thread primitives.

## Runtime checking

The canonical bootstrap tool exposes:

```text
noqeri run app.nqr --check-memory
noqeri run app.nqr --overflow
noqeri test tests --race
```

`--race` enables the first-generation thread-aware memory conflict detector. Raw non-atomic accesses are tracked by address and thread, while spawn/join, channels, locks and atomics create synchronization boundaries. It is useful now and has direct multi-threaded regression coverage; higher-precision vector-clock analysis and larger stress campaigns remain hardening work rather than syntax requirements.
