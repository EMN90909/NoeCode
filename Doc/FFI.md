# FFI, embedding and bindings

Noqeri separates two contracts that must not be confused.

## Stable embedding ABI: ABI v2

`Include/noqeri/abi.h` is the stable C-facing embedding boundary. Hosts negotiate `noqeri_abi_version()`, populate the ABI-v2 service table, and execute source through `noqeri_run_source`. The ABI is versioned and structures use `struct_size` so compatible fields can be appended without silently changing the meaning of older fields.

Ownership rules are intentionally explicit:

- `noqeri_abi_string` is a pointer plus byte length; it does not imply NUL termination.
- Arguments supplied to a host callback are borrowed for the duration of that callback. Copy them before retaining them.
- A string returned from a host service remains provider-owned long enough for the runtime to copy it immediately; ownership is not transferred to the runtime unless a future API explicitly says so.
- `noqeri_abi_error.message` is borrowed. Copy it if the host needs it after the call boundary.
- status `0` means success; non-zero values represent failure. Do not overload pointer/null conventions as hidden error channels.
- allocator ownership never crosses the boundary implicitly. The component that allocates memory owns its release unless an ABI function documents a matching release operation.

These conventions apply to C hosts, dynamic-library hosts and future language bindings around ABI v2.

## Native symbols are not yet the portable C ABI

The current native backend can emit/consume `extern function` and `export function` symbols, but the x86-64 backend still uses Noqeri's own native calling convention in places. Symbol visibility is therefore **not** proof of a stable C ABI for arbitrary exported Noqeri functions.

Until a backend-specific C-compatible shim implements parameter/return lowering, aggregate layout, callbacks, error translation and ownership on every supported platform, native symbol bindings remain experimental. Do not publish a generated header as proof that a Noqeri function can be called safely from arbitrary C code.

## Binding generation

`Tools/ffi/noqeri-bindgen.mjs` reads a deliberately small JSON interface and emits a Noqeri declaration file plus a C header. Native C prototypes are placed behind `NOQERI_EXPERIMENTAL_NATIVE_EXPORT_ABI` so users must opt into the unfinished native-export contract.

Example:

```json
{
  "schema": 1,
  "module": "demo.host",
  "imports": [
    {"name":"host_now_ms","params":[],"returns":"i64"}
  ],
  "exports": [
    {"name":"demo_add","params":[{"name":"a","type":"int"},{"name":"b","type":"int"}],"returns":"int"}
  ]
}
```

Generate bindings with:

```sh
node Tools/ffi/noqeri-bindgen.mjs examples/ffi/interface.json --out=build/ffi
```

The initial generator accepts scalar ABI-friendly types only: `void`, `bool`, `int`, `i64`, `u64`, `usize`, `isize`, `float`, `f64`, and `string`. Records/structs are intentionally rejected until stable layout/alignment rules are implemented rather than guessed.

## Remaining stable-FFI gate

Before Noqeri can advertise arbitrary native functions as stable C ABI, the backend must have cross-platform conformance tests for calling convention, integer/float registers, stack alignment, struct layout/padding, callbacks, string ownership, errors, dynamic libraries and Windows/Linux/macOS symbol behavior. Header generation is tooling; those backend tests are the compatibility proof.
