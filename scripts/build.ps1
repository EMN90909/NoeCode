$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$BuildDir = if ($env:BUILD_DIR) { $env:BUILD_DIR } else { Join-Path $Root 'build' }
$Stage0 = $env:NOQERI_STAGE0
$Bootstrap = Join-Path $Root 'Compiler\selfhost\bootstrap.nqr'

if (-not (Get-Command node -ErrorAction SilentlyContinue)) { Write-Error 'Noqeri bootstrap tooling currently requires Node.js 20+' }
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

if (-not $Stage0) {
  $found = Get-Command noqeri -ErrorAction SilentlyContinue
  if ($found) { $Stage0 = $found.Source }
}

# If a trusted Noqeri executable is unavailable, compile the checked-out C++
# bootstrap source locally. This removes the preinstalled-binary requirement;
# Stage-2 equivalence still gates removal of the C++ trust root itself.
if (-not $Stage0) {
  if ($env:NOQERI_ALLOW_SOURCE_BOOTSTRAP -eq '0') { Write-Error 'Noqeri seed is missing and source bootstrap is disabled.' }
  if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { Write-Error 'No Noqeri stage-0 was found. Install CMake/C++ for automatic source bootstrap or set NOQERI_STAGE0.' }
  $SeedBuild = Join-Path $BuildDir 'stage0-seed'
  $PathFile = Join-Path $BuildDir 'stage0-seed.path'
  $Proof = Join-Path $BuildDir 'stage0-seed.json'
  & node (Join-Path $Root 'scripts\source-bootstrap.mjs') "--root=$Root" "--build-dir=$SeedBuild" "--path-file=$PathFile" "--proof=$Proof"
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  $Stage0 = (Get-Content -Path $PathFile -TotalCount 1).Trim()
}

if (-not (Test-Path $Stage0)) { Write-Error "Noqeri stage-0 does not exist: $Stage0" }
& $Stage0 check $Bootstrap
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$Module = Join-Path $BuildDir 'noqeri-stage1.nqo'
& $Stage0 web $Bootstrap $Module
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$Launcher = Join-Path $BuildDir 'noqeri.cmd'
$Cli = Join-Path $Root 'Runtime\stage1-cli.mjs'
Set-Content -Path $Launcher -Encoding ascii -Value "@echo off`r`nset NOQERI_STAGE1_MODULE=$Module`r`nnode `"$Cli`" %*`r`n"
& $Launcher selftest
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $Launcher --version
Write-Host "Noqeri stage-1 built from Noqeri bootstrap source at $Launcher"
if ($env:NOQERI_STAGE0) { Write-Host "Stage-0 provenance: external seed $Stage0" }
else { Write-Host "Stage-0 provenance: $(Join-Path $BuildDir 'stage0-seed.json')" }
