# Roadmap

## 1.0 hardening

Increase parser/type-system conformance tests, complete formatter coverage, expand LSP capabilities, add fuzz targets and lock down diagnostic stability.

## Native backends

Keep Linux x86-64 stable, then implement PE/COFF on Windows and Mach-O on macOS behind the same NIR contracts.

## Modules and standard library

Connect import semantics, module discovery, package resolution and the standard-library surface to the compiler rather than adding unimplemented library files.

## Self-hosting

Grow Ric until the compiler can be implemented in `.ric`, then maintain a reproducible staged bootstrap so users do not need another language after the bootstrap boundary.

## Tooling

Turn `editors/vscode/` into a publishable extension package, expand `ric lsp`, add debugger protocol support and generate API docs from compiler metadata.
