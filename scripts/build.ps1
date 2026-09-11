$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path build | Out-Null
$files = Get-ChildItem compiler/bootstrap/*.cpp | ForEach-Object { $_.FullName }

$gppCandidates = @(
  "C:\msys64\ucrt64\bin\g++.exe",
  "C:\msys64\mingw64\bin\g++.exe"
)
foreach ($candidate in $gppCandidates) {
  if (Test-Path $candidate) {
    & $candidate -std=c++17 -O2 -Wall -Wextra -Iinclude/noe $files -o build/noe.exe
    if ($LASTEXITCODE -ne 0) { throw "g++ bootstrap build failed" }
    Write-Host "Built build/noe.exe with $candidate"
    exit 0
  }
}

if (Get-Command g++ -ErrorAction SilentlyContinue) {
  g++ -std=c++17 -O2 -Wall -Wextra -Iinclude/noe $files -o build/noe.exe
  if ($LASTEXITCODE -ne 0) { throw "g++ bootstrap build failed" }
  Write-Host "Built build/noe.exe with g++"
  exit 0
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
  $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
  if ($vs) {
    $vcvars = Join-Path $vs "VC\Auxiliary\Build\vcvars64.bat"
    if (Test-Path $vcvars) {
      $fileArgs = ($files | ForEach-Object { '"' + $_ + '"' }) -join ' '
      $cmd = "call `"$vcvars`" && cl /nologo /std:c++17 /EHsc /O2 /Iinclude/noe $fileArgs /Fe:build\noe.exe"
      cmd /c $cmd
      if ($LASTEXITCODE -ne 0) { throw "MSVC bootstrap build failed" }
      Write-Host "Built build/noe.exe with MSVC"
      exit 0
    }
  }
}

throw "Install MSYS2 g++, MinGW g++, or MSVC Build Tools to build the one-time C++ bootstrap compiler."
