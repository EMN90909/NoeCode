# macOS platform

macOS is supported as a host for the Noe bootstrap compiler frontend and tooling.

Supported now:

- build the Noe compiler with a C++17 compiler
- run `noe check`, `noe run`, `noe test`, `noe format`, `noe lsp`
- use `project.noe` and `noe.lock`

Build and test:

```sh
sh scripts/build.sh
sh scripts/test_core.sh
```

Planned native target work:

- Mach-O object/executable support
- macOS x64 and ARM64 ABI support
- codesigning/notarization documentation for distributed builds
