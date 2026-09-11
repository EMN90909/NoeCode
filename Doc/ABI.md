# Noqeri host ABI v1

Noqeri keeps the stable source language deliberately small. Native capability is provided through a versioned host boundary instead of pointer syntax, raw-memory instructions, architecture intrinsics or layout-dependent records.

## Language-side model

Ordinary services look like ordinary calls:

```nqr
print(platform())
print(clockMillis())
print(textLength("noqeri"))
```

Embedders can expose additional capabilities without extending the grammar:

```nqr
let result = host("app.lookup", "customer-42")
print(result)
```

`host` is not a new expression form. It lowers to the existing NIR `Call` instruction. The first argument is a string service name; remaining arguments cross the ABI as primitive tagged values.

## ABI contract

The public C header is `Include/noqeri/abi.h`. ABI v1 contains only:

- an explicit `NOQERI_ABI_VERSION`;
- tagged `null`, `bool`, `int`, `float` and `string` values;
- length-delimited strings;
- a function entry `{ name, invoke, user_data }`;
- a host table with an ABI version and entry count;
- `noqeri_abi_version()` and `noqeri_run_source()` entry points.

No AST, C++ object, STL type, NIR structure, pointer arithmetic contract or compiler-internal layout is part of the public ABI.

## Minimal host

```c
#include <noqeri/abi.h>

static int32_t answer(void* user_data,
                      const noqeri_abi_value* args,
                      size_t argc,
                      noqeri_abi_value* result,
                      noqeri_abi_error* error) {
    (void)user_data; (void)args; (void)argc; (void)error;
    result->tag = NOQERI_ABI_INT;
    result->as.integer = 42;
    return 0;
}

int main(void) {
    const noqeri_abi_function_entry functions[] = {
        {"app.answer", answer, NULL},
    };
    const noqeri_host_api host = {
        NOQERI_ABI_VERSION,
        1,
        functions,
    };
    noqeri_abi_error error = {0};
    return noqeri_run_source("print(host(\"app.answer\"))", &host, &error);
}
```

## Built-in services

The reference runtime currently exposes `print`, `platform`, `clockMillis` and `textLength` through the same host-call mechanism. This removes the previous interpreter-only special case for `print` and gives future standard services one extension point.

## Native backend

The bootstrap Linux x86-64 backend remains freestanding. Programs that use runtime host services should use `noqeri run` or embed the shared `noqeri-abi` library. The CLI rejects host-dependent programs before native emission instead of silently producing unresolved external symbols.

## Compatibility rule

ABI additions must be backward compatible within ABI v1. Any breaking change to value representation, calling convention, ownership rules or table layout requires a new ABI version. The source-language grammar can therefore remain stable while host capability evolves independently.
