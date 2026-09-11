# Windows bootstrap build

From PowerShell with CMake and a C++17 toolchain available:

```powershell
.\scripts\build.ps1
ctest --test-dir build -C Release --output-on-failure
.\build\Release\ric.exe doctor .
```

The exact binary subdirectory depends on the CMake generator. The script handles configuration/build; CI validates a Windows runner.
