# Windows platform

Windows is supported as a host for the Noe bootstrap compiler frontend and tooling.

Supported now:

- build `noe.exe` with MSVC Build Tools or g++
- run `noe check`, `noe run`, `noe test`, `noe format`, `noe lsp`
- use `project.noe` and `noe.lock`

Build and test:

```powershell
.\scripts\build.ps1
.\scripts\test_core.ps1
```

Planned native target work:

- Windows x64 code-generation rules
- PE/COFF object output
- MSVC or custom linker integration
- Windows installer and `.noe` file association
