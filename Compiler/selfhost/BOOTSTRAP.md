# Noqeri bootstrap ownership

`bootstrap.nqr` is the source entry point for a normal stage-1 build. Compiler policy, lexical rules, syntax checks, type-width policy, package-coordinate validation, and package archive limits are Noqeri-owned.

A trusted stage-0 executable is still necessary for the first translation of `bootstrap.nqr` into `noqeri-stage1.nqo`. That is a bootstrap trust boundary, not a C/C++ production dependency. The historical C++ seed remains on `bootstrap/cpp17-stage0` for reproducibility.

The portable `.nqo` object currently uses Node.js only as a host for filesystem/process/network capabilities. Those host capabilities must not own language semantics. Removing that host is a later native-backend milestone and should not be claimed complete until stage1 builds stage2 and equivalence tests pass.
