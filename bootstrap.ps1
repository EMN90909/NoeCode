$ErrorActionPreference = 'Stop'
$script = Join-Path $PSScriptRoot 'scripts\build.ps1'
& $script
exit $LASTEXITCODE
