# noqeri 1.4 language reference

noqeri remains a compact statically checked language. Version 1.4 expands the same small core with general systems and library capabilities: arrays/slices, modules/imports, lightweight error propagation, register-value generics, atomics, CPU intrinsics, and constrained inline assembly. None of these features implies an operating-system profile.

## Scalar types

The portable-width integer types are:

- unsigned: `u8`, `u16`, `u32`, `u64`, `usize`
- signed: `i8`, `i16`, `i32`, `i64`, `isize`

`usize` and `isize` are pointer-sized. On the current x86-64 native target they are 64-bit.

The convenient types `int`, `float`, `bool`, `string`, `null`, and `void` remain available. `int` is the ordinary general-purpose signed integer type.

## Arrays and slices

Fixed arrays use `[T; N]`:

```nqr
let values: [u32; 4] = [1 as u32, 2 as u32, 3 as u32, 4 as u32]
print(values[2])
```

Array literals infer one element type. Noqeri does not silently narrow an `int` literal into `u32`; cast when fixed-width storage matters.

Slices use `[]T` and carry a data pointer plus length:

```nqr
let view: []u32 = slice(values)
print(len(view))
print(view[0])
```

A slice can also be created from raw storage:

```nqr
let view: []u8 = slice(bufferPointer, bufferLength)
```

`len(array)` is compile-time constant when the array size is known. `len(slice)` reads the slice descriptor length. Arrays and slice descriptors use explicit stack allocation in the current freestanding backend, so they require no heap or OS runtime.

## Pointers

```nqr
let value: u32 = 42
let p: *u32 = &value
let copy: u32 = *p
p[0] = 7
```

`*T` is a pointer to `T`. `&value` produces an address. Unary `*` dereferences. Indexing performs scaled pointer arithmetic using the size of `T`.

Explicit pointer/integer conversion uses `as`:

```nqr
let address: usize = 0x1000
let mmio: *u32 = address as *u32
let raw: usize = mmio as usize
```

Implicit integer-to-pointer conversion is intentionally not performed.

## Volatile memory

Volatile is attached to the pointer:

```nqr
let status: *volatile u32 = 0xF0000000 as *volatile u32
let current: u32 = status[0]
status[1] = current
```

Loads and stores through a `*volatile T` lower to volatile NIR memory operations. `volatile` is a memory-access promise; it is not a synchronization primitive. Use atomics for inter-thread synchronization.

## Records

```nqr
record PixelBuffer {
    address: *volatile u32
    width: u32
    height: u32
    pitch: u32
}
```

Records use declaration-order layout with natural alignment. Field offsets are resolved during type checking, so native code sees explicit address calculations rather than dynamic property lookup.

A pointer to a record uses the same field syntax:

```nqr
function clear(buffer: *PixelBuffer): void {
    buffer.address[0] = 0
}
```

The `.` operator automatically treats a pointer-to-record as the base address for field access. No separate `->` operator is required.

## Generics

The first generic form is intentionally small and predictable: inferred function type parameters for register-sized values.

```nqr
function identity<T>(value: T): T {
    return value
}

let a: u32 = identity(7 as u32)
let b: *u8 = identity(pointer)
```

Type arguments are inferred from call arguments; there is no explicit call-site specialization syntax. The bootstrap compiler uses one register-oriented body rather than generating a separate copy for each scalar/pointer type.

This first generic model is meant for values that fit the current calling convention. Generic aggregate specialization and generic record layout are intentionally separate future extensions rather than hidden behavior.

## Modules and imports

A source file may declare an organizational module name:

```nqr
module graphics.raster
```

Files are imported by relative path:

```nqr
import "math.nqr"
```

The CLI recursively resolves imports relative to the importing file, deduplicates already-loaded files, and compiles the resulting declaration graph together. The current bootstrap keeps names flat after import; the file syntax does not lock Noqeri into a future namespace design.

`compileSource` remains useful for embedding single source strings. File-backed CLI commands use `compileFile` so imports are actually resolved rather than merely parsed.

## Lightweight error handling

Noqeri 1.4 provides allocation-free status propagation for integer-returning functions:

```nqr
function readValue(): isize {
    if device_failed {
        throw 3
    }
    return 42
}

function caller(): isize {
    let value: isize = try readValue()
    return value
}
```

`throw code` converts a non-negative error code into a negative status and returns immediately. `try expression` propagates a negative status from the current function; non-negative values continue normally.

This is deliberately a low-cost checked-status model, not exceptions backed by unwinding or an allocator. It works in applications, libraries, embedded code and freestanding environments. Rich sum-type errors can be added independently later.

## Atomics

Atomic operations are ordinary typed calls:

```nqr
let counter: u64 = 0
let p: *u64 = &counter

atomic.store(p, 1)
let current: u64 = atomic.load(p)
let old: u64 = atomic.exchange(p, 2)
let observed: u64 = atomic.compareExchange(p, 2, 3)
atomic.fence()
```

The bootstrap accepts integer, boolean, and pointer values up to 8 bytes. The x86-64 backend lowers these operations directly using memory-ordering instructions and locked read/modify/write operations. The default ordering is sequential consistency; no OS service is involved.

Camel-case aliases (`atomicLoad`, `atomicStore`, and so on) are accepted for simple embedding/generated code, but the namespaced spelling is preferred in source.

## CPU intrinsics

Architecture-specific operations use one explicit escape hatch:

```nqr
intrinsic("x86.pause")
let cycles: u64 = intrinsic("x86.rdtsc")
intrinsic("x86.halt")
intrinsic("compiler.fence")
```

The intrinsic name makes architecture dependence visible in source. Unsupported intrinsics are compile-time errors rather than silently turning into host calls.

The reference interpreter treats non-destructive intrinsics as reference operations; `x86.halt` is rejected there because halting the host process would be incorrect.

## Inline assembly

For operations that do not justify a named intrinsic yet:

```nqr
asm("nop")
```

The current x86-64 backend deliberately restricts `asm` to one instruction string and rejects labels, newlines, and assembler directives. This keeps inline assembly an escape hatch without allowing one expression to silently rewrite sections or symbols around the compiler.

The reference interpreter treats inline assembly as a no-op, because its purpose is validating language/runtime behavior rather than emulating CPU instructions.

## Casts

`value as Type` is the one explicit cast form. Numeric casts, pointer-to-pointer casts, `usize`/`isize` to pointer, pointer to `usize`/`isize`, and `null` to pointer are supported.

## Linkage

```nqr
extern function device_write(data: *u8, count: usize): isize

export function library_entry(data: *u8, count: usize): isize {
    return device_write(data, count)
}
```

`extern` declares a function whose symbol is provided by the surrounding link. `export` makes the Noqeri function available under its declared name. Neither keyword implies an operating system.

## NIR memory and concurrency model

The NIR has explicit operations for:

- `address_of`
- width-carrying `load_memory` / `store_memory`
- volatile loads/stores
- `ptr_offset`
- `stack_alloc`
- `make_slice`, `slice_data`, `slice_len`
- `cast`
- atomic load/store/exchange/compare-exchange/fence
- `intrinsic`
- `asm`
- `try` / `throw`

These operations remain visible through lowering so the native backend does not need to infer low-level intent from ordinary function calls.

## ABI services

The Noqeri ABI remains separate from raw language memory and concurrency. Friendly services such as `print`, `clockMillis`, and `platform` can still be supplied through the Noqeri ABI. Low-level code does not need those services to use arrays, slices, pointers, records, atomics, exports, extern declarations, intrinsics, or inline assembly.

See `Doc/ABI.md` and `Doc/FREESTANDING.md` for embedding and target details.
