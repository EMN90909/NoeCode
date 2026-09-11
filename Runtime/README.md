# Runtime

Ric's bootstrap runtime is the NIR interpreter plus the platform native backend/linker path. Runtime responsibilities include built-ins such as `print`, call frames, value representation, process exit status and future allocation/GC behavior.

Runtime behavior must be shared semantically between interpreted and native execution; backend-specific differences should be treated as bugs unless explicitly documented.
