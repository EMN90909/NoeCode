$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$BuildDir = if ($env:BUILD_DIR) { $env:BUILD_DIR } else { Join-Path $Root 'build' }
$Stage0 = $env:NOQERI_STAGE0
$Bootstrap = Join-Path $Root 'Compiler\selfhost\bootstrap.nqr'
if (-not $Stage0) {
  $found = Get-Command noqeri -ErrorAction SilentlyContinue
  if ($found) { $Stage0 = $found.Source }
}
if (-not $Stage0) {
  Write-Error "Noqeri's bootstrap program is written in Noqeri. Set NOQERI_STAGE0 to a trusted Noqeri seed for the first stage. CMake/C++ are not used by this normal build."
}
if (-not (Get-Command node -ErrorAction SilentlyContinue)) { Write-Error 'The portable .nqo host currently requires Node.js 20+' }
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
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
