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

- task/channel/mutex/RW-lock operations establish synchronization edges;
- sequentially-consistent atomics establish a total order for the participating atomic operations;
- cancellation is cooperative and must be observed at cancellation points;
- structured task scopes must not return while child tasks remain unjoined unless they were explicitly detached;
- unsynchronized conflicting memory accesses from different threads are a data race and outside the safe-language contract.

Race detection is a planned tool mode rather than a silent runtime behavior. The intended command is `noqeri test --race` once instrumentation is available.
