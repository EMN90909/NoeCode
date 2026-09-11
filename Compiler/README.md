# noqeri compiler

`bootstrap/` contains the buildable C++17 compiler used to establish noqeri. It owns lexical analysis, parsing, type checking, NIR lowering/optimization, interpreted execution, native Linux x86-64 emission, project tooling, formatter, tests and LSP bootstrap behavior.

The long-term goal is a reproducible staged bootstrap with the compiler written in `.nqr`; until then this directory is the canonical implementation rather than a set of placeholders.
