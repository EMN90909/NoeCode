# Platform support

This directory is the cross-platform ownership index for noqeri, mirroring the purpose of a mature platform layer without copying CPython implementation.

| Platform | Bootstrap frontend/interpreter/tooling | Native executable backend |
|---|---|---|
| Linux x86-64 | CI-supported | implemented smoke path |
| Windows x64 | CI-supported | planned PE/COFF backend |
| macOS arm64/x86-64 | CI-supported | planned Mach-O backend |
| Android | design/integration track | not release-ready |

Platform-specific notes live in `PC/`, `PCbuild/`, `Mac/` and `Android/`. See `Doc/PLATFORM_SUPPORT.md` for user-facing support policy.
