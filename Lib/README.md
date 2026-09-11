# Ric standard library

`Lib/` is the standard-library source area. Only library code that the current grammar/runtime can actually parse is checked in. Module/import wiring is tracked in `Modules/` and is not represented here by fake APIs.

The current seed modules provide small pure-Ric functions useful for compiler conformance. As imports become implemented, these files will gain stable module identities and tests.
