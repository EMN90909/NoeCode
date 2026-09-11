# Developer tools

Build, test and release entry points live in `scripts/`. Editor integration lives in `editors/`. Compiler-integrated tools include `noqeri format`, `noqeri test`, `noqeri doctor` and `noqeri lsp`.

`Tools/build/`, `Tools/fuzz/` and `Tools/scripts/` document ownership for build/release automation, fuzzing and repository maintenance. They should gain implementation files only when those files perform real work.
