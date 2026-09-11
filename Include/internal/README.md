# Internal header policy

Installed/public bootstrap declarations live in `Include/noqeri/`. Compiler-private headers should move under this directory as subsystem boundaries stabilize. The current bootstrap still uses one compatibility shim in `Compiler/bootstrap/noe.hpp`; it is intentionally not installed and is not part of the noqeri API.
