$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path build | Out-Null
$files = Get-ChildItem compiler/bootstrap/*.cpp | ForEach-Object { $_.FullName }
if (Get-Command cl -ErrorAction SilentlyContinue) {
  cl /std:c++17 /EHsc /O2 /Iinclude/noe $files /Fe:build/noe.exe
} elseif (Get-Command g++ -ErrorAction SilentlyContinue) {
  g++ -std=c++17 -O2 -Wall -Wextra -Iinclude/noe $files -o build/noe.exe
} else {
  throw "Install MSVC Build Tools or g++ to build the one-time C++ bootstrap compiler."
}
Write-Host "Built build/noe.exe"
