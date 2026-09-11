$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path build | Out-Null

if (Get-Command cmake -ErrorAction SilentlyContinue) {
  cmake -S . -B build/cmake -DCMAKE_BUILD_TYPE=Release
  cmake --build build/cmake --config Release
  $candidate = Get-ChildItem build/cmake -Recurse -Filter noe.exe | Select-Object -First 1
  if (-not $candidate) { throw "CMake build completed but noe.exe was not found under build/cmake." }
  Copy-Item $candidate.FullName build/noe.exe -Force
} elseif (Get-Command g++ -ErrorAction SilentlyContinue) {
  $files = Get-ChildItem compiler/bootstrap/*.cpp | ForEach-Object { $_.FullName }
  g++ -std=c++17 -O2 -Wall -Wextra -Iinclude/noe $files -o build/noe.exe
} else {
  throw "Install CMake with MSVC Build Tools or g++ to build the one-time C++ bootstrap compiler."
}

Write-Host "Built build/noe.exe"
