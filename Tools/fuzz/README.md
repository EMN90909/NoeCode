# Fuzzing

Fuzzing is a production-release gate for noqeri. The first target is source ingestion: arbitrary bytes must never crash the lexer/parser/type-check pipeline outside intentional process limits.

The normal build does not require libFuzzer. A dedicated sanitizer/fuzz target should compile against `compileSource` with `-fsanitize=fuzzer,address,undefined` on supported Clang toolchains. Corpus inputs should be minimized `.nqr` programs drawn from `examples/` and `tests/`.

Until this target is continuously exercised, `Doc/PRODUCTION_READINESS.md` correctly lists broader fuzzing as unfinished work.
