param(
    [switch]$Yes,
    [switch]$SkipInstall
)

$script = Join-Path $PSScriptRoot 'scripts\build.ps1'
& $script -Yes:$Yes -SkipInstall:$SkipInstall
exit $LASTEXITCODE
