$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$BuildDir = if ($env:BUILD_DIR) { $env:BUILD_DIR } else { Join-Path $Root 'build' }
$Stage0 = $env:NOQERI_STAGE0
$Compiler = Join-Path $Root 'Compiler\selfhost\main.nqr'
if (-not $Stage0) {
  $found = Get-Command noqeri -ErrorAction SilentlyContinue
  if ($found) { $Stage0 = $found.Source }
}
if (-not $Stage0) {
  if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { Write-Error 'No NOQERI_STAGE0 was supplied and CMake is unavailable for clean source bootstrap.' }
  New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
  $Stage0Dir = Join-Path $BuildDir 'stage0-provenance'
  & cmake -S $Root -B $Stage0Dir -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  & cmake --build $Stage0Dir --config Release --target noqeri --parallel
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  $Candidates = @((Join-Path $Stage0Dir 'noqeri.exe'),(Join-Path $Stage0Dir 'Release\noqeri.exe'),(Join-Path $Stage0Dir 'noqeri'),(Join-Path $Stage0Dir 'Release\noqeri'))
  $Stage0 = $Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
  if (-not $Stage0) { Write-Error 'Source bootstrap completed but the provenance compiler executable was not found.' }
}
if (-not (Get-Command node -ErrorAction SilentlyContinue)) { Write-Error 'The portable Stage-2 host currently requires Node.js 20+' }
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
& $Stage0 check $Compiler
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$Module = Join-Path $BuildDir 'noqeri-stage2.nqo'
& $Stage0 web $Compiler $Module
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$Launcher = Join-Path $BuildDir 'noqeri.cmd'
$Cli = Join-Path $Root 'Runtime\stage2-cli.mjs'
Set-Content -Path $Launcher -Encoding ascii -Value "@echo off`r`nset NOQERI_STAGE2_MODULE=$Module`r`nnode `"$Cli`" %*`r`n"
& $Launcher selftest
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $Launcher stage2-status
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $Launcher --version
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "Noqeri Stage-2 pipeline artifact built from Noqeri compiler source at $Launcher"
Write-Host "Clean bootstrap seed: $Stage0"
Write-Host 'Bootstrap equivalence remains governed separately by scripts/bootstrap-stage2.mjs.'
