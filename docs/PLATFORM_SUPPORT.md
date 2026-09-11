# Platform support

NoeCode is now organized and tested as a cross-platform bootstrap compiler project.

## Supported host platforms

| Host platform | Build bootstrap compiler | Check/type-check `.noe` | Interpret `.noe` | Format/test/project tools | LSP diagnostics |
|---|---:|---:|---:|---:|---:|
| Linux x86-64 | yes | yes | yes | yes | yes |
| macOS | yes | yes | yes | yes | yes |
| Windows | yes | yes | yes | yes | yes |

## Native executable targets

| Target | Status |
|---|---|
| Linux x86-64 ELF | implemented for the bootstrap subset |
| Windows x64 PE/COFF | planned |
| macOS ARM64/x64 Mach-O | planned |

The current native backend intentionally produces Linux x86-64 assembly and links it with GNU binutils. Windows and macOS can still build and use the Noe compiler for lexing, parsing, checking, NIR output, formatting, project tooling, tests, LSP diagnostics and interpreted execution.

## Build commands

Linux/macOS:

```sh
sh scripts/build.sh
sh scripts/test_core.sh
```

Windows PowerShell:

```powershell
.\scripts\build.ps1
.\scripts\test_core.ps1
```

Optional CMake build on any platform with a C++17 compiler:

```sh
cmake -S . -B build/cmake
cmake --build build/cmake --config Release
```

## Full native release check

The full native release check currently runs on Linux x86-64:

```sh
sh scripts/release_check.sh
```

It verifies the compiler, interpreter, diagnostics, formatter, project tools, LSP diagnostics, custom native backend, linker driver and generated executable.
