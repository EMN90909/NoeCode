# Parser

`Parser/` owns the executable front-end implementation for noqeri syntax.

- `lexer.cpp` tokenizes `.nqr` source.
- `parser.cpp` builds the bootstrap AST defined by `Include/noqeri/noqeri.hpp`.
- `Grammar/noqeri.grammar.md` is the user-visible syntax contract.

Grammar changes must update the grammar contract, lexer/parser behavior, language reference and conformance tests together.
