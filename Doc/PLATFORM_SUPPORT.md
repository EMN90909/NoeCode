# Platform support

| Capability | Linux x86-64 | Windows | macOS |
|---|---:|---:|---:|
| Build bootstrap compiler | yes | yes | yes |
| `ric lex/check/format` | yes | yes | yes |
| `ric run` interpreter | yes | yes | yes |
| `ric test` | yes | yes | yes |
| `ric lsp` | yes | yes | yes |
| native executable backend | ELF x86-64 | planned PE/COFF | planned Mach-O |

The repository does not label Windows or macOS native code generation as complete. Their supported production-track surface is the compiler frontend, interpreter and tooling.

Platform-specific organization is in `PC/`, `PCbuild/`, `Mac/` and `Android/`.
