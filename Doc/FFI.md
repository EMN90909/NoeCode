# Noqeri FFI contract

Status: **preview** until callback/native-layout conformance is complete. The embedding ABI in `Doc/ABI.md` remains independently versioned.

## Design rule

FFI is an opt-in systems feature. Ordinary applications should consume safe packages instead of declaring native functions directly.

## C ABI declarations

`extern function` declares a native/provider boundary. Calling a raw extern function requires `unsafe` unless a standard/provider wrapper contains that boundary.

Primitive mappings for generated C bindings are:

| Noqeri | C |
| --- | --- |
| `void` | `void` |
| `bool` | `bool` |
| `i8/u8` | `int8_t/uint8_t` |
| `i16/u16` | `int16_t/uint16_t` |
| `i32/u32` | `int32_t/uint32_t` |
| `i64/u64` | `int64_t/uint64_t` |
| `isize/usize` | `intptr_t/uintptr_t` |
| `int` | `int64_t` in the 2026 edition |
| `float` | `double` |
| raw `*T` | `T*` |

Noqeri `string` and slices are **not** silently mapped to NUL-terminated C pointers. Crossing that boundary requires an explicit ABI record or generated adapter because length and ownership matter.

## Struct layout

A record used in stable C FFI must use the target ABI layout documented by the compiler: declaration-order fields, target alignment and no undocumented packing. Until explicit representation attributes and layout conformance are finalized, generated bindings reject records rather than guessing.

## Ownership

Every pointer-returning native API must document one of:

- **borrowed:** caller does not free and must not outlive the owner;
- **caller-owned:** caller releases using the named matching release function;
- **provider-owned:** provider retains ownership until a documented lifecycle event;
- **static:** valid for process/provider lifetime and immutable unless stated otherwise.

Ownership never transfers merely because a raw pointer crossed the ABI.

## Errors

Preferred C-facing convention for fallible operations is an integer status plus explicit out-parameters. `0` means success unless the API documents a stronger convention; negative values are reserved for Noqeri/provider failure families. Do not expose language exceptions across a C ABI.

The embedding ABI uses `noqeri_abi_error` and status returns. Higher-level Noqeri wrappers should translate low-level status values into the ordinary Noqeri error model.

## Callbacks

Native callbacks require an explicit function pointer plus `void*`/context value so callers do not depend on hidden closures. Callback lifetime must be documented. A callback must not be invoked after its registration/context has been released.

Callback lowering is a release gate and remains preview until native and interpreter conformance tests agree across Windows, Linux and macOS.

## Dynamic libraries

Dynamic-library loading is a host capability. Library search paths must never implicitly include the current working directory in secure/default mode. Production packages should prefer explicit paths or platform loader policy and must verify architecture/ABI compatibility before use.

## Binding generation

Use `scripts/gen-c-header.mjs` for the supported Noqeri-to-C primitive subset and `scripts/gen-noqeri-bindings.mjs` for the supported C-to-Noqeri declaration subset. Both generators fail closed on unsupported types rather than emitting a plausible but ABI-incorrect declaration.
