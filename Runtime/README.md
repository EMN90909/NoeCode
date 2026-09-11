# Runtime

noqeri's bootstrap runtime is the NIR interpreter plus the platform native backend/linker path. Runtime responsibilities include built-ins such as `print`, call frames, value representation, process exit status and future allocation/GC behavior.

Interpreted and native execution should share semantics; backend-specific differences are bugs unless explicitly documented.
