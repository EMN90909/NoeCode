# CPython structure study and noqeri mapping

CPython is used here as a maturity reference for **separation of responsibilities**, not as source code to copy. Its current root explicitly separates `Doc`, `Grammar`, `Include`, `InternalDocs`, `Lib`, `Mac`, `Misc`, `Modules`, `Objects`, `PC`, `PCbuild`, `Parser`, `Platforms`, `Programs`, `Python`, and `Tools`, alongside build configuration and CI metadata.

noqeri maps those responsibilities to language-appropriate homes:

| CPython responsibility | noqeri home |
|---|---|
| grammar | `Grammar/` |
| public C API headers | `Include/noqeri/` bootstrap API |
| parser implementation | `Parser/` |
| core interpreter/compiler implementation (`Python/`) | `Compiler/` + `Runtime/` |
| built-in/native modules | `Modules/` |
| runtime object model | `Objects/` |
| standard library | `Lib/` |
| executable programs | `Programs/` |
| developer and generated tooling | `Tools/` |
| public docs | `Doc/` |
| internal engineering docs | `InternalDocs/` |
| Windows/macOS/platform integration | `PC/`, `PCbuild/`, `Mac/`, `Platforms/`, `Android/` |
| release/project records | `Misc/` |
| broad tests/performance work | `tests/`, `Lib/test/`, `Benchmarks/` |

The repository also follows CPython's engineering habits where they fit noqeri: platform CI, explicit security/contribution policy, a documented build, install targets, release checks, code ownership, editor/tooling separation and a repository-doctor check.

The important lesson is not directory-name cosplay. A mature repository makes each responsibility discoverable, testable and documented. noqeri therefore avoids empty implementation placeholders: a directory that does not yet own compiled code must still contain a concrete contract, policy, test, benchmark or integration guide.

Structural completeness does **not** mean feature parity with CPython. CPython has decades of implementation and ecosystem work; noqeri's exact implemented subset and missing release gates remain documented in `Doc/PRODUCTION_READINESS.md`.
