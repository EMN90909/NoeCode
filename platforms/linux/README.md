# Linux platform

Linux x86-64 is the first complete native executable target for the bootstrap compiler.

Supported now:

- build the Noe compiler with a C++17 compiler
- run `noe check`, `noe run`, `noe test`, `noe format`, `noe lsp`
- emit x86-64 assembly from NIR
- assemble/link an ELF executable using GNU binutils

Full verification:

```sh
sh scripts/release_check.sh
```
