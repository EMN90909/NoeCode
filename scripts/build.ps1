$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$BuildDir = if ($env:BUILD_DIR) { $env:BUILD_DIR } else { Join-Path $Root 'build' }
$Stage0 = $env:NOQERI_STAGE0
if (-not $Stage0) {
  $found = Get-Command noqeri -ErrorAction SilentlyContinue
  if ($found) { $Stage0 = $found.Source }
}
if (-not $Stage0) {
  Write-Error "Noqeri needs a trusted stage-0 compiler for bootstrap. Set NOQERI_STAGE0 or use a seed built from branch bootstrap/cpp17-stage0. CMake/C++ are not used by this normal build."
}
if (-not (Get-Command node -ErrorAction SilentlyContinue)) { Write-Error 'Noqeri stage-1 portable host requires Node.js 20+' }
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
& $Stage0 check (Join-Path $Root 'Compiler/selfhost/main.nqr')
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$Module = Join-Path $BuildDir 'noqeri-stage1.nqo'
& $Stage0 web (Join-Path $Root 'Compiler/selfhost/main.nqr') $Module
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$Launcher = Join-Path $BuildDir 'noqeri.cmd'
$Cli = Join-Path $Root 'Runtime/stage1-cli.mjs'
Set-Content -Path $Launcher -Encoding ascii -Value "@echo off`r`nset NOQERI_STAGE1_MODULE=$Module`r`nnode `"$Cli`" %*`r`n"
& $Launcher selftest
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $Launcher --version
Write-Host "Noqeri stage-1 built at $Launcher"
