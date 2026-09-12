$ErrorActionPreference = 'Stop'
$required = @(
  'README.md','LICENSE','LEGAL.md','SECURITY.md','CONTRIBUTING.md','CMakeLists.txt','project.nqr','noqeri.lock',
  'Brand/noqeri-logo.webp','Brand/noqeri-mark.png','Grammar/noqeri.grammar.md','Include/noqeri/noqeri.hpp',
  'Parser/lexer.cpp','Parser/parser.cpp','Parser/ascii.nqr','Parser/token_rules.nqr','Compiler/core.cpp','Compiler/semantic_rules.nqr',
  'Runtime/interpreter.cpp','Runtime/status.nqr','Runtime/memory.nqr','Runtime/limits.nqr',
  'Programs/noqeri.cpp','Programs/selftest.nqr','Programs/diagnostics.nqr','Programs/version.nqr',
  'Modules/builtin.nqr','Modules/native.nqr','Objects/value.nqr','Objects/text.nqr','Database/noqeridb.nqr',
  'Tools/security/policy.nqr','Tools/fuzz/generator.nqr','Tools/build/policy.nqr','Tools/scripts/source_floor.nqr',
  'Lib/security/memory.nqr','Lib/test/assert.nqr','Lib/std/int.nqr','Lib/std/slice_i64.nqr','Lib/std/search.nqr','Lib/std/sort.nqr','Lib/std/stats.nqr','Lib/std/bytes.nqr','Lib/std/utf8.nqr','Lib/std/checksum.nqr','Lib/std/memory.nqr','Lib/std/status.nqr','Lib/std/time.nqr','Lib/std/random.nqr','Lib/std/matrix_i64.nqr','Lib/std/range.nqr','Lib/std/text.nqr','Lib/std/set_i64.nqr','Lib/std/map_i64.nqr','Lib/std/stack_i64.nqr','Lib/std/queue_i64.nqr','Lib/std/algorithm.nqr','Lib/std/bit.nqr',
  'tests/noqeri_owned_components.nqr','tests/noqeri_library_suite.nqr','editors/vscode/package.json','site/index.html'
)
foreach ($path in $required) { if (-not (Test-Path $path)) { throw "missing required repository path: $path" } }

$placeholders = @('Modules/README.md','Objects/README.md','Lib/README.md','Lib/test/README.md','Android/README.md','Mac/README.md','PC/README.md','PCbuild/README.md','Platforms/README.md','Parser/README.md','Runtime/README.md','Programs/README.md','Benchmarks/README.md','Compiler/README.md','Tools/README.md','Tools/build/README.md','Tools/fuzz/README.md')
foreach ($path in $placeholders) { if (Test-Path $path) { throw "implementation placeholder must be executable .nqr, not $path" } }

$legacy = Get-ChildItem -Recurse -File | Where-Object { $_.Extension -eq '.noe' -or $_.Extension -eq '.ric' }
if ($legacy) { throw "legacy source extensions found: $($legacy.FullName -join ', ')" }

$noqeriOwned = @('Modules','Objects','Lib','Database','Tools/security','Tools/fuzz','Platforms','PC','Mac','Android','PCbuild')
$nativeLibrarySource = Get-ChildItem $noqeriOwned -Recurse -File | Where-Object { $_.Extension -in @('.c','.cc','.cpp','.h','.hpp') }
if ($nativeLibrarySource) {
  throw "C/C++ source found in Noqeri-owned implementation directories: $($nativeLibrarySource.FullName -join ', '). Move host/bootstrap adapters under Compiler/bootstrap and keep reusable implementation in .nqr"
}

$noqeriCount = (Get-ChildItem -Recurse -File -Filter '*.nqr').Count
$stdlibCount = (Get-ChildItem 'Lib/std' -Recurse -File -Filter '*.nqr').Count
if ($noqeriCount -lt 30) { throw "Noqeri source floor failed: $noqeriCount < 30" }
if ($stdlibCount -lt 16) { throw "Noqeri stdlib floor failed: $stdlibCount < 16" }
Write-Host "repository structure verified: Noqeri source=$noqeriCount stdlib=$stdlibCount; C++ confined away from Noqeri-owned library areas"
