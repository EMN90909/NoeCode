# Contributing to NoeCode

Thanks for helping build Noe.

Noe is still a bootstrap language implementation. Contributions should keep the project aligned with the design contract: simple syntax, static typing, explicit safety boundaries, native capability, reproducible tooling, text/visual/AI-ready structure, and no TOML for Noe projects.

## Priorities

1. Keep the compiler pipeline working: lexer -> parser -> type checker -> NIR -> optimizer -> interpreter/native backend.
2. Add tests before or with compiler changes.
3. Prefer small, reviewable changes over large rewrites.
4. Do not claim unsupported language features are complete.
5. Keep Noe project metadata in `project.noe`.

## Local verification

```sh
sh scripts/test.sh
```

## Style

- Bootstrap code targets C++17.
- Noe examples should stay simple and readable.
- Diagnostics should use stable `NOE-*` codes.
- Documentation must clearly separate implemented features from roadmap features.

## License

Unless explicitly stated otherwise, contributions are submitted under the repository license in `LICENSE`.
