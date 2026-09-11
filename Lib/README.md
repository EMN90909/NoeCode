# noqeri standard library

`Lib/` is the standard-library source area. Only code that the current grammar/runtime can actually parse is checked in. Import/module wiring is tracked in `Modules/` and is not represented by fake APIs.

The current seed files provide small pure-noqeri functions useful for compiler conformance. `Lib/test/` documents how library behavior is promoted into the conformance suite once imports are implemented.
