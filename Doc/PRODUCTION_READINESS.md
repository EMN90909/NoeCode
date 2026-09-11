# Production readiness

Ric 1.0 is on a production track, not yet a claim of full production maturity.

## Gates already represented in the repository

- deterministic build entry points and CI on Linux, Windows and macOS;
- lexer/parser/type-check/NIR/interpreter pipeline;
- Linux x86-64 native backend smoke path;
- `.ric` test discovery;
- project/lock format;
- diagnostics, formatter and LSP foundation;
- license, security policy, contribution rules and changelog;
- editor syntax package and static public site.

## Gates still required for a stable production release

- broader conformance, negative and fuzz testing;
- import/module semantics connected to the parser and runtime;
- Windows PE/COFF and macOS Mach-O native backends;
- package registry/signature/trust design;
- stable diagnostic and compiler API policy;
- performance and memory regression suites;
- self-hosting milestone or a formally supported bootstrap strategy;
- reproducible release artifacts and signed checksums.

`ric doctor` verifies repository structure. It is not a substitute for the missing technical release gates above.
