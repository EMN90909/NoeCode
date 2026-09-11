# Windows bootstrap build

```powershell
.\scripts\build.ps1
ctest --test-dir build -C Release --output-on-failure
.\build\Release\noqeri.exe doctor .
```

The exact binary subdirectory depends on the CMake generator. CI validates a Windows runner.
