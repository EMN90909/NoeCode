# Parser subsystem

The buildable parser currently lives at `compiler/bootstrap/parser.cpp`; this directory owns parser architecture, generated-parser policy and future self-hosted parser sources.

The parser consumes tokens from `Lexer`, builds the AST types declared in `Include/ric/ric.hpp`, reports structured diagnostics and follows `Grammar/ric.grammar.md`. Grammar changes require parser and test changes in the same commit.
