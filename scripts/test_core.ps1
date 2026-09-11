$ErrorActionPreference = 'Stop'
& "$PSScriptRoot\build.ps1"
$Ric = Join-Path (Get-Location) 'build\Release\ric.exe'
if (-not (Test-Path $Ric)) { $Ric = Join-Path (Get-Location) 'build\ric.exe' }
& $Ric --version
& $Ric check 'examples\hello.ric'
& $Ric run 'examples\hello.ric'
& $Ric test 'tests'
& $Ric doctor '.'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
