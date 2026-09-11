# Parser subsystem

The buildable recursive-descent parser currently lives at `Compiler/bootstrap/parser.cpp`. This directory owns grammar-to-parser policy, parser error-recovery design and future generated/self-hosted parser work.

Grammar changes require parser, formatter, editor grammar and `.nqr` test changes in the same commit.
