# noqeri 1.3 language reference

noqeri remains a compact statically checked language, but 1.3 adds a small set of explicit systems primitives. They are general-purpose features for libraries, embedded programs, runtimes, FFI, high-performance code, and kernels; there is no special OS dialect or kernel mode.

## Scalar types

The portable-width integer types are:

- unsigned: `u8`, `u16`, `u32`, `u64`, `usize`
- signed: `i8`, `i16`, `i32`, `i64`, `isize`

`usize` and `isize` are pointer-sized. On the current x86-64 native target they are 64-bit.

The existing convenient types `int`, `float`, `bool`, `string`, `null`, and `void` remain available. `int` is the ordinary general-purpose signed integer type.

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

Loads and stores through a `*volatile T` lower to volatile NIR memory operations. `volatile` is a memory-access promise; it is not a general variable modifier.

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

A pointer to a record may use ordinary field syntax:

```nqr
function clear(buffer: *PixelBuffer): void {
    buffer.address[0] = 0
}
```

The `.` operator automatically treats a pointer-to-record as the base address for field access. This keeps common systems code simple without adding `->`.

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

## NIR memory model

The NIR now has explicit operations for:

- `address_of`
- `load_memory`
- `store_memory`
- volatile memory loads/stores
- `ptr_offset`
- `cast`

Memory operations carry an access width. Volatile operations remain visible to the optimizer.

## ABI services

The Noqeri ABI remains separate from raw language memory. Friendly services such as `print`, `clockMillis`, and `platform` can still be supplied through the Noqeri ABI. Low-level code does not need those services to use pointers, records, exports, or extern declarations.

See `Doc/ABI.md` and `Doc/FREESTANDING.md` for embedding and target details.
