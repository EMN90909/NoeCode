$ErrorActionPreference = 'Stop'
$required = @(
  'README.md','LICENSE','LEGAL.md','SECURITY.md','CONTRIBUTING.md','CMakeLists.txt','project.nqr','noqeri.lock',
  'Brand/noqeri-logo.webp','Brand/noqeri-mark.png','Grammar/noqeri.grammar.md','Include/noqeri/noqeri.hpp',
  'Parser/lexer.cpp','Parser/parser.cpp','Compiler/core.cpp','Runtime/interpreter.cpp','Programs/noqeri.cpp',
  'Tools/build/production.cpp','editors/vscode/package.json','site/index.html'
)
foreach ($path in $required) { if (-not (Test-Path $path)) { throw "missing required repository path: $path" } }
$legacy = Get-ChildItem -Recurse -File | Where-Object { $_.Extension -eq '.noe' -or $_.Extension -eq '.ric' }
if ($legacy) { throw "legacy source extensions found: $($legacy.FullName -join ', ')" }
Write-Host 'repository structure verified: noqeri / .nqr'
