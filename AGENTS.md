# Agent guidance

Ric is a language implementation, not an application repository.

Before changing syntax, read `Grammar/ric.grammar.md`, `Doc/LANGUAGE_REFERENCE.md`, and the compiler header in `Include/ric/ric.hpp`. Keep lexer, parser, type checker, NIR, interpreter/native backend and tests aligned. Do not create empty placeholder files. Prefer small changes with executable tests. Public language spelling is `Ric`, executable `ric`, source extension `.ric`, manifest `project.ric`, lockfile `ric.lock`.

Run the build and test scripts for the target platform, then run `ric doctor .` before release-oriented changes.
