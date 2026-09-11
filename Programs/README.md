# Programs

`Programs/` owns user-facing noqeri tools built into the compiler executable.

- `noqeri.cpp` — CLI entry point.
- `formatter.cpp` — source formatter.
- `package.cpp` — project/lock tooling.
- `lsp.cpp` — JSON-RPC language-server foundation.
- `test_runner.cpp` — `.nqr` test runner.

Keeping these tools separate from parser/compiler/runtime code makes command behavior easier to evolve without collapsing the repository into one source directory.
