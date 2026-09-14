# Noqeri bootstrap ownership

`bootstrap.nqr` is the source entry point for the portable stage-1 build. Compiler policy, lexical rules, syntax checks, type-width policy, package-coordinate validation, and package archive limits are Noqeri-owned.

## No preinstalled Noqeri binary is required

`scripts/build.sh` and `scripts/build.ps1` first prefer an explicitly supplied `NOQERI_STAGE0`, then an installed `noqeri`. If neither exists they now build the checked-out C++ bootstrap compiler locally with CMake and use that executable to translate `Compiler/selfhost/bootstrap.nqr` into `build/noqeri-stage1.nqo`.

The source-bootstrap path writes `build/stage0-seed.json`, including the seed SHA-256, bootstrap-source SHA-256, build commands and repository commit when available. This makes the first-stage provenance inspectable instead of requiring an opaque binary to be provided out of band.

Set `NOQERI_ALLOW_SOURCE_BOOTSTRAP=0` when a build policy requires an explicitly supplied trusted seed and must not invoke the C++ source bootstrap automatically.

## What this does and does not prove

The automatic source bootstrap fixes the practical chicken-and-egg problem: a clean machine can build Noqeri from this repository with a C++17 compiler, CMake and Node.js 20+, without first installing Noqeri.

It does **not** by itself remove C++ from the bootstrap trust root. The local C++ seed remains trusted for the first translation. C++ can be removed from the release trust story only after executable evidence shows:

1. stage-0 builds stage-1;
2. stage-1 builds stage-2 from the complete self-hosted compiler source closure;
3. stage-2 rebuilds the compiler;
4. the two independently produced stage-2 outputs are byte-identical or pass an explicitly defined semantic-equivalence check; and
5. the required frontend, NIR, optimizer, object-writer, formatter, LSP, package-manager, fuzz, race, memory, overflow, reproducibility and practical-corpus suites pass.

`scripts/bootstrap-stage2.mjs` remains the gate for that evidence. A source-bootstrap proof is therefore provenance for stage-0, while a successful stage-2 proof is what permits the stronger self-hosting claim.

The portable `.nqo` object currently uses Node.js as a host for filesystem/process/network capabilities. Those host capabilities must not own language semantics. Removing that host is a later native-backend milestone and should not be claimed complete until stage-2 reproduction and equivalence tests pass.
