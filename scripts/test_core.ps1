$ErrorActionPreference = 'Stop'
& "$PSScriptRoot\build.ps1"
$Noqeri = Join-Path (Get-Location) 'build\Release\noqeri.exe'
if (-not (Test-Path $Noqeri)) { $Noqeri = Join-Path (Get-Location) 'build\noqeri.exe' }
& $Noqeri --version
& $Noqeri check 'examples\hello.nqr'
& $Noqeri run 'examples\hello.nqr'
& $Noqeri test 'tests'
& $Noqeri doctor '.'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
