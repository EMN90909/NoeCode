# Noe Toolchain Reference

The main tool is `noe`.

## Build the compiler

```sh
sh scripts/build.sh
```

## Commands

```text
noe --version
noe new <name>
noe check <file>
noe lex <file>
noe nir <file>
noe run <file>
noe build <file> <output>
noe format <file>
noe manifest [project.noe]
noe lock [project.noe]
noe test [directory]
noe lsp
```

## Command summary

- `noe new` creates a project using `project.noe`.
- `noe check` runs parsing and type checking.
- `noe nir` prints optimized NIR.
- `noe run` executes through the reference interpreter.
- `noe build` produces a Linux x86-64 native executable for the supported subset.
- `noe lock` generates deterministic lock metadata.
- `noe format` applies canonical formatting foundations.
- `noe test` compiles and runs `.noe` tests.
- `noe lsp` starts the language-server bootstrap.
