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

## `noe new`

Creates a project using Noe-native metadata:

```text
project.noe
src/main.noe
```

## `noe check`

Runs the frontend and type checker without executing code.

## `noe nir`

Prints optimized NIR for inspection and debugging.

## `noe run`

Compiles to NIR and executes through the reference interpreter.

## `noe build`

Compiles the supported subset to a Linux x86-64 native executable.

## `noe lock`

Generates deterministic lock metadata for the current project. Remote package resolution is future work.

## `noe format`

Applies the canonical formatter foundation to a `.noe` source file.

## `noe test`

Finds `.noe` files in a test directory, compiles them, and runs them.

## `noe lsp`

Starts the language-server bootstrap. It supports JSON-RPC initialization and compiler diagnostics for editor/agent integration foundations.
