# Compiler

`Compiler/` owns noqeri semantic analysis and code generation. The compiled bootstrap implementation now lives here rather than in one catch-all bootstrap directory.

- `core.cpp` — diagnostics, type utilities and compile pipeline coordination.
- `type_checker.cpp` — static checking.
- `nir.cpp` — noqeri IR lowering/printing.
- `optimizer.cpp` — NIR optimization.
- `native_backend.cpp` — native assembly emission.
- `linker.cpp` — platform linker driver.
- `bootstrap/noe.hpp` — temporary compatibility include used by the C++ bootstrap sources; the public header is `Include/noqeri/noqeri.hpp`.

New compiler implementation should be placed by responsibility, with tests and documentation updated in the same change.
