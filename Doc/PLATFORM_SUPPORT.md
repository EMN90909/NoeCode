# Platform support

| Capability | Linux x86-64 | Windows | macOS |
|---|---:|---:|---:|
| Build bootstrap compiler | yes | yes | yes |
| `noqeri lex/check/format` | yes | yes | yes |
| `noqeri run` interpreter | yes | yes | yes |
| `noqeri test` | yes | yes | yes |
| `noqeri lsp` | yes | yes | yes |
| native executable backend | ELF x86-64 | planned PE/COFF | planned Mach-O |

Platform-specific ownership is in `PC/`, `PCbuild/`, `Mac/` and `Android/`.
