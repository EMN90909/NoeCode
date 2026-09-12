$ErrorActionPreference = 'Stop'
$required = @(
  'README.md','LICENSE','LEGAL.md','SECURITY.md','CONTRIBUTING.md','CMakeLists.txt','project.nqr','noqeri.lock',
  'Brand/noqeri-logo.webp','Brand/noqeri-mark.png','Grammar/noqeri.grammar.md','Include/noqeri/noqeri.hpp',
  'Parser/lexer.cpp','Parser/parser.cpp','Compiler/core.cpp','Runtime/interpreter.cpp','Programs/noqeri.cpp',
  'Tools/build/production.cpp','Modules/native.nqr','Objects/value.nqr','Objects/text.nqr','Database/noqeridb.nqr',
  'Tools/security/policy.nqr','Lib/test/assert.nqr','tests/noqeri_owned_components.nqr','editors/vscode/package.json','site/index.html'
)
foreach ($path in $required) { if (-not (Test-Path $path)) { throw "missing required repository path: $path" } }
$legacy = Get-ChildItem -Recurse -File | Where-Object { $_.Extension -eq '.noe' -or $_.Extension -eq '.ric' }
if ($legacy) { throw "legacy source extensions found: $($legacy.FullName -join ', ')" }
$noqeriOwned = @('Modules','Objects','Lib','Database','Tools/security')
$nativeLibrarySource = Get-ChildItem $noqeriOwned -Recurse -File | Where-Object { $_.Extension -in @('.c','.cc','.cpp','.h','.hpp') }
if ($nativeLibrarySource) {
  throw "C/C++ source found in Noqeri-owned library/runtime directories: $($nativeLibrarySource.FullName -join ', '). Move host/bootstrap adapters under Compiler/bootstrap and keep reusable implementation in .nqr"
}
Write-Host 'repository structure verified: compiler bootstrap + Noqeri-owned runtime/library components'
