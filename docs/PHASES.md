# Noe bootstrap phases

The 8 bootstrap phases are a path from source text to a tested native executable. They are deliberately narrower than the long-term Noe language roadmap.

1. **Frontend** — lexer, tokens, source spans and parser/AST.
2. **Semantics** — lexical scopes, primitive static types, inference and structured diagnostics.
3. **Portable execution core** — NIR lowering, optimization and reference interpreter.
4. **Native backend** — Linux x86-64 assembly generation behind NIR.
5. **Link/build** — assembler/linker driver and `noe build` producing an ELF executable.
6. **Project/package foundation** — `project.noe`, `noe new`, manifest parsing and deterministic `noe.lock`.
7. **Developer tooling** — canonical formatter, test runner and JSON-RPC language server diagnostics.
8. **Verification** — end-to-end smoke suite and CI covering the complete bootstrap pipeline.

## Phase 8 acceptance test

`sh scripts/test.sh` must prove all of the following in one clean run:

- the C++17 bootstrap compiler builds with warnings enabled;
- valid Noe source passes lexical, parse and type analysis;
- invalid typed source is rejected with a stable `NOE-T` diagnostic;
- source lowers to NIR and executes in the reference interpreter;
- the formatter produces canonical spacing;
- `.noe` tests are discovered and executed;
- the custom x86-64 backend emits assembly;
- the linker driver creates a native ELF executable;
- the native executable produces the same expected output as the interpreter;
- `noe new`, `project.noe` and `noe.lock` work without TOML;
- the language server speaks framed JSON-RPC and publishes compiler diagnostics.

The bootstrap remains intentionally honest about unsupported native operations. Unsupported float code generation or runtime string binary operations produce diagnostics rather than incorrect machine code.
