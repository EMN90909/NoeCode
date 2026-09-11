# Release process

A release candidate must pass CI on Linux, Windows and macOS, Linux native smoke tests, `noqeri doctor`, `.nqr` conformance tests and documentation review. Release notes must state unsupported native targets honestly. Stable releases should eventually publish reproducible platform artifacts plus checksums/signatures; that artifact pipeline is a release gate, not currently claimed complete.
