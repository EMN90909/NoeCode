# Build tooling ownership

CMake is the canonical bootstrap build. Platform-specific integration belongs in `PCbuild/` and `Mac/`; release scripts should call the same compiler/test targets rather than creating divergent build logic.
