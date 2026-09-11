# Contributing to NoeCode

Thanks for helping build Noe.

Noe is still a bootstrap language implementation. Contributions should keep the project aligned with the design contract: simple syntax, static typing, explicit unsafe boundaries, native capability, reproducible tooling, text/visual/AI-ready structure, and no TOML for Noe projects.

## Current contribution priorities

1. Keep the existing compiler pipeline working: lexer -> parser -> type checker -> NIR -> optimizer -> interpreter/native backend.
2. Add tests before or with compiler changes.
3. Prefer small, reviewable changes over large rewrites.
4. Do not claim unsupported language features are complete.
5. Keep Noe project metadata in `project.noe`; do not add TOML to Noe projects.

## Local verification

```sh
sh scripts/test.sh
```

The test script builds the bootstrap compiler, runs interpreter tests, compiles the native smoke program, executes the generated binary, verifies project creation, checks deterministic `noe.lock`, and exercises LSP diagnostics.

## Style

- C++ bootstrap code should target C++17.
- Noe examples should use readable, simple syntax.
- Diagnostics should use stable `NOE-*` codes where possible.
- Documentation should clearly separate implemented features from roadmap features.

## License

Unless explicitly stated otherwise, contributions are submitted under the repository license in `LICENSE`.
