# Production readiness

noqeri 1.0 is on a production track, not yet a claim of full production maturity.

Already represented: cross-platform bootstrap CI, lexer/parser/type-check/NIR/interpreter pipeline, Linux x86-64 native smoke path, `.nqr` tests, project/lock format, diagnostics, formatter, LSP foundation, public policies, editor support and repository-doctor checks.

Still required for a stable production release: broader positive/negative conformance and fuzzing; implemented import/module semantics; PE/COFF and Mach-O native backends; package registry/signature/trust design; stable diagnostic/compiler API policy; performance/memory regression suites; reproducible signed release artifacts; and a self-hosted compiler or formally supported long-term bootstrap strategy.

`noqeri doctor` validates repository structure. It does not pretend the unfinished gates are complete.
