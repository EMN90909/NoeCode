# Compiler architecture

`Compiler/bootstrap/` is the currently buildable compiler. Source is lexed into tokens, parsed into an AST, statically checked, lowered to NIR, optimized and then passed to either the reference interpreter or native backend.

`Include/noqeri/noqeri.hpp` is the public bootstrap API contract. The implementation retains one internal compatibility namespace/include name from the earliest bootstrap; those names are not part of the `.nqr` language surface or installed API and can be removed at a deliberate compiler-ABI break.

Subsystem ownership is separated into `Parser/`, `Objects/`, `Modules/`, `Runtime/`, `Programs/` and `Tools/` so future implementation does not collapse into an undifferentiated source tree.
