# Compiler architecture

`compiler/bootstrap/` is the currently buildable compiler. Source is lexed into tokens, parsed into an AST, statically checked, lowered to NIR, optimized and then passed to either the interpreter or native backend.

`Include/ric/ric.hpp` is the public bootstrap API contract. The bootstrap implementation still uses an internal compatibility namespace inherited from the pre-Ric codebase; external code should use the `ric` namespace alias and Ric public names. This compatibility layer can be removed at an ABI-breaking compiler milestone without affecting `.ric` language syntax.

`Parser/`, `Python/`, `Objects/`, `Modules/` and `Runtime/` document subsystem ownership so future implementation does not collapse back into a single undifferentiated source directory.
