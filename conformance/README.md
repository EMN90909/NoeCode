# Noqeri conformance suite

This directory is compiler behavior, not examples.

- `valid/` contains programs that every conforming compiler must accept.
- `invalid/` contains programs that every conforming compiler must reject.
- backend-observable programs belong under `differential/` and must produce the same exit code/stdout on every backend that supports the required capabilities.

Run locally:

```sh
NOQERI_BIN=./build/noqeri node scripts/conformance.mjs
```

The runner intentionally does not invoke GitHub Actions or any network service. Release automation may call the same local runner, but the test semantics remain repository-owned.

Invalid cases should be small and target one rule. When a stable diagnostic code/status exists, add a sibling `<file>.expect` containing the required code or status fragment. A diagnostic wording improvement may then occur without weakening the semantic assertion.
