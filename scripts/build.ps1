$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$BuildDir = if ($env:BUILD_DIR) { $env:BUILD_DIR } else { Join-Path $Root 'build' }
$Stage0 = $env:NOQERI_STAGE0
$Bootstrap = Join-Path $Root 'Compiler\selfhost\bootstrap.nqr'

if (-not $Stage0) {
  $found = Get-Command noqeri -ErrorAction SilentlyContinue
  if ($found) { $Stage0 = $found.Source }
}

# No external Noqeri executable is required for a clean checkout. Build the
# checked-in C++ bootstrap compiler, then use it once to create portable stage 1.
if (-not $Stage0) {
  if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Error 'No NOQERI_STAGE0 was supplied and CMake is unavailable for source bootstrap.'
  }
  New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
  $Stage0Dir = Join-Path $BuildDir 'stage0'
  & cmake -S $Root -B $Stage0Dir -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  & cmake --build $Stage0Dir --config Release --target noqeri --parallel
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  $Candidates = @(
    (Join-Path $Stage0Dir 'noqeri.exe'),
    (Join-Path $Stage0Dir 'Release\noqeri.exe'),
    (Join-Path $Stage0Dir 'noqeri'),
    (Join-Path $Stage0Dir 'Release\noqeri')
  )
  $Stage0 = $Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
  if (-not $Stage0) { Write-Error 'Source bootstrap completed but the stage-0 compiler executable was not found.' }
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
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "Noqeri stage-1 built from source bootstrap at $Launcher"
