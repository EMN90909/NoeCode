# Ric toolchain

The bootstrap pipeline is:

`source -> lexer -> parser/AST -> type checker -> NIR -> optimizer -> interpreter or native backend`

The executable is `ric`.

- `ric lex` inspects lexical tokens.
- `ric check` parses and type-checks.
- `ric nir` prints optimized NIR.
- `ric run` executes NIR in the reference interpreter.
- `ric build` emits assembly and links a Linux x86-64 executable in the current backend.
- `ric format` formats source in place.
- `ric test` discovers `.ric` test programs recursively.
- `ric lsp` runs the JSON-RPC language-server foundation over stdio.
- `ric doctor` validates repository release structure.

CMake is the canonical bootstrap build. The language project format itself remains Ric-native (`project.ric`), not CMake.
