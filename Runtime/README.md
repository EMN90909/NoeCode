# Runtime

`Runtime/` owns noqeri execution semantics and the future native runtime/ABI boundary.

`interpreter.cpp` is the current reference NIR interpreter. It is used for `noqeri run`, tests and cross-platform semantic validation while native backends mature.

Native runtime work must document allocation, strings, calls, errors/panics, platform ABI behavior and compatibility guarantees before those interfaces are treated as stable.
