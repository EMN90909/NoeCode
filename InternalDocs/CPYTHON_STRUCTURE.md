# CPython structure study and noqeri mapping

CPython is used here as a maturity reference for **separation of responsibilities**, not as source code to copy. Its root separates areas such as `Doc`, `Grammar`, `Include`, `InternalDocs`, `Lib`, `Mac`, `Misc`, `Modules`, `Objects`, `PC`, `PCbuild`, `Parser`, `Programs`, `Python`, `Tools`, platform support and extensive CI/configuration.

noqeri maps those responsibilities to language-appropriate homes: public docs in `Doc/`; grammar in `Grammar/`; public headers in `Include/noqeri/`; compiler internals in `Compiler/`; internal design docs in `InternalDocs/`; standard library in `Lib/`; runtime values in `Objects/`; native/built-in module boundary in `Modules/`; parser ownership in `Parser/`; executable contracts in `Programs/`; developer tooling in `Tools/`; platform work in `PC`, `PCbuild`, `Mac` and `Android`; and release/project records in `Misc/`.

The important lesson is not directory-name cosplay. A mature repository makes each responsibility discoverable, testable and documented. noqeri therefore avoids empty placeholder implementation files: directories that do not yet own compiled code contain explicit contracts, release criteria or integration instructions instead.

Structural completeness does **not** mean feature parity with CPython. CPython has decades of implementation and ecosystem work; noqeri's exact implemented subset remains documented in `Doc/PRODUCTION_READINESS.md`.
