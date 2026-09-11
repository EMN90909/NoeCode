# CPython structure study and Ric mapping

The Ric repository uses CPython as a maturity reference for **separation of responsibilities**, not as source code to copy.

At the time of this reorganization, CPython's root separates major areas such as `Doc`, `Grammar`, `Include`, `InternalDocs`, `Lib`, `Mac`, `Misc`, `Modules`, `Objects`, `PC`, `PCbuild`, `Parser`, `Programs`, `Python` and `Tools`, alongside CI/configuration files. Ric now has corresponding responsibility areas adapted to a compiled language.

| CPython-style responsibility | Ric location |
|---|---|
| user docs | `Doc/` |
| grammar | `Grammar/` |
| public implementation headers | `Include/ric/` |
| internal implementation docs | `InternalDocs/` |
| standard library | `Lib/` |
| built-in/native modules | `Modules/` |
| runtime objects/value model | `Objects/` |
| parser | `Parser/` + `compiler/bootstrap/parser.cpp` |
| executable entry points | `Programs/` + bootstrap `main.cpp` |
| core compiler/runtime implementation | `Python/` + `compiler/bootstrap/` |
| developer tools | `Tools/` |
| Windows integration | `PC/`, `PCbuild/` |
| Apple integration | `Mac/` |
| mobile target notes | `Android/` |
| miscellaneous release/project records | `Misc/` |

The key lesson is not to create thousands of empty files. Completeness means each responsibility has a real owner, build/test path and documentation, while implementation files are added only when they do real work.
