## Summary

Describe the Noe compiler, docs, tests, or tooling change.

## Checks

- [ ] `sh scripts/test_core.sh` passes on Linux/macOS or `.\scripts	est_core.ps1` passes on Windows.
- [ ] `sh scripts/release_check.sh` passes for Linux native backend changes.
- [ ] New language behavior is documented in `docs/` or `grammar/`.
- [ ] No TOML, Cargo, Rust, Python, or LLVM dependency was introduced.

## Notes

Mention unsupported platform behavior honestly when a change touches native code generation.
