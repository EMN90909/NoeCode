# Repository structure review

The NoeCode repository was reorganized after reviewing the shape of `python/cpython` as an example of a mature language implementation repository.

## What CPython demonstrates

A mature language repo is not only source files. It has clearly separated areas for documentation, grammar, public headers, implementation code, standard library code, platform-specific build support, developer tools, examples/tests, CI and release policy.

CPython's root layout includes major areas such as `.github`, `Doc`, `Grammar`, `Include`, `Lib`, `Modules`, `Objects`, `Parser`, `Programs` and `Tools`. Noe does not copy CPython's implementation or use Python; it adapts the completeness pattern to Noe's own architecture.

## NoeCode equivalent

| Mature-language concern | NoeCode path |
|---|---|
| Compiler implementation | `compiler/bootstrap/` |
| Public bootstrap header/API | `include/noe/` |
| Grammar notes | `grammar/` |
| Standard library seed | `stdlib/std/` |
| Runtime design | `runtime/` |
| Platform notes | `platforms/` |
| Developer tools | `tools/` |
| User and implementer docs | `docs/` |
| Examples | `examples/` |
| Tests | `tests/` |
| CI | `.github/workflows/` |
| Build scripts | `scripts/`, `CMakeLists.txt`, `Makefile` |

## Design rule

The structure should look professional without pretending unfinished systems are complete. Directories contain real documentation or working code. Unsupported features remain documented as planned work until implemented and tested.
