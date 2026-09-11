# Runtime

This directory is reserved for Noe runtime components.

The current bootstrap supports a small built-in runtime surface: values, function calls, variables, branching, loops and printing. As Noe grows, runtime work will move here instead of being hidden inside compiler stages.

Planned runtime areas:

- memory management for the app profile
- deterministic resource cleanup
- strings and UTF handling
- arrays and collections
- async task runtime
- platform I/O
- FFI boundary support

The system profile will keep runtime features optional so low-level targets can opt out.
