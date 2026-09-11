# Noe bootstrap architecture

Noe 0.0.3 is the first executable compiler foundation. The bootstrap implementation is C++17 only; it intentionally uses no Rust, and Noe projects use no TOML.

Implemented and executable in phase 3/8:

- lexer with source spans, comments, literals, keywords and operators;
- recursive-descent parser for variables, functions, blocks, calls, expressions, `if`, `while` and `return`;
- static type checker with inference for primitive values, lexical scopes, function signatures and structured diagnostics;
- register-based NIR lowering;
- constant-folding optimizer foundation;
- NIR reference interpreter;
- `project.noe` manifest reader;
- canonical formatter foundation;
- test runner;
- CLI commands for `lex`, `check`, `nir`, `run`, `format`, `manifest` and `test`.

Present as explicit subsystem boundaries but intentionally not claimed complete at phase 3/8:

- custom native machine-code backend;
- linker driver;
- remote package download/resolution and `noe.lock` generation;
- JSON-RPC language-server transport and document synchronization.

The next milestone is native object generation and linking behind NIR without changing the language frontend. The long-term compiler will progressively move from the one-time C++ bootstrap into self-hosted `.noe` source.
