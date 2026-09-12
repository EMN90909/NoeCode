param(
    [switch]$Yes,
    [switch]$SkipInstall
)

$ErrorActionPreference = 'Stop'

$StartedAt = Get-Date
$RepoRoot = Split-Path -Parent $PSScriptRoot
$BannerPath = Join-Path $RepoRoot 'Brand\noqeri-banner.txt'
$BuildDir = if ($env:BUILD_DIR) { $env:BUILD_DIR } else { 'build' }
$BuildType = if ($env:BUILD_TYPE) { $env:BUILD_TYPE } else { 'Release' }

function Show-NoqeriBanner {
    if (Test-Path $BannerPath) {
        Write-Host (Get-Content -Path $BannerPath -Raw -Encoding UTF8)
    }
}

function Format-Elapsed {
    param([Parameter(Mandatory = $true)][TimeSpan]$Elapsed)
    return ('{0:00}:{1:00}:{2:00}' -f [int]$Elapsed.TotalHours, $Elapsed.Minutes, $Elapsed.Seconds)
}

function Show-NextSteps {
    param([Parameter(Mandatory = $true)][string]$Compiler)
    Write-Host ''
    Write-Host 'How to use:'
    Write-Host "  & `"$Compiler`" --version"
    Write-Host "  & `"$Compiler`" run examples\hello.nqr"
    Write-Host "  & `"$Compiler`" check examples\hello.nqr"
    Write-Host "  ctest --test-dir $BuildDir -C $BuildType --output-on-failure"
}

function Test-Command {
    param([Parameter(Mandatory = $true)][string]$Name)
    return $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

function Refresh-ProcessPath {
    $current = $env:Path
    $machine = [Environment]::GetEnvironmentVariable('Path', 'Machine')
    $user = [Environment]::GetEnvironmentVariable('Path', 'User')
    $parts = @()
    if ($current) { $parts += $current }
    if ($machine) { $parts += $machine }
    if ($user) { $parts += $user }
    if ($parts.Count -gt 0) { $env:Path = $parts -join ';' }

    $cmakeBin = Join-Path $env:ProgramFiles 'CMake\bin'
    if ((Test-Path $cmakeBin) -and ($env:Path -notlike "*$cmakeBin*")) {
        $env:Path = "$cmakeBin;$env:Path"
    }
}

function Test-MsvcBuildTools {
    $vswhereCandidates = @()
    if (${env:ProgramFiles(x86)}) {
        $vswhereCandidates += (Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe')
    }
    if ($env:ProgramFiles) {
        $vswhereCandidates += (Join-Path $env:ProgramFiles 'Microsoft Visual Studio\Installer\vswhere.exe')
    }

    foreach ($vswhere in $vswhereCandidates) {
        if (-not (Test-Path $vswhere)) { continue }
        $installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
        if ($LASTEXITCODE -eq 0 -and $installation) { return $true }
    }
    return $false
}

function Test-CxxToolchain {
    if (Test-Command 'cl.exe') { return $true }
    if (Test-Command 'clang++.exe') { return $true }
    if (Test-Command 'g++.exe') { return $true }
    return Test-MsvcBuildTools
}

function Get-MissingPrerequisites {
    $missing = @()
    if (-not (Test-Command 'cmake.exe')) { $missing += 'CMake' }
    if (-not (Test-CxxToolchain)) { $missing += 'C++ build toolchain (MSVC Build Tools, clang++, or g++)' }
    return $missing
}

function Invoke-WingetInstall {
    param([Parameter(Mandatory = $true)][string[]]$Arguments)
    & winget.exe @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "winget failed with exit code $LASTEXITCODE"
    }
}

function Install-MissingPrerequisites {
    param([Parameter(Mandatory = $true)][string[]]$Missing)

    if (-not (Test-Command 'winget.exe')) {
        Write-Warning 'Automatic installation requires Windows Package Manager (winget).'
        Write-Host 'Install App Installer/winget, or install CMake and a C++ toolchain manually, then rerun this script.'
        return $false
    }

    if ($Missing -contains 'CMake') {
        Write-Host 'Installing CMake with winget...'
        Invoke-WingetInstall @('install', '--id', 'Kitware.CMake', '--exact', '--accept-package-agreements', '--accept-source-agreements')
    }

    if ($Missing -contains 'C++ build toolchain (MSVC Build Tools, clang++, or g++)') {
        Write-Host 'Installing Visual Studio 2022 Build Tools with the C++ workload...'
        Invoke-WingetInstall @(
            'install',
            '--id', 'Microsoft.VisualStudio.2022.BuildTools',
            '--exact',
            '--accept-package-agreements',
            '--accept-source-agreements',
            '--override', '--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended'
        )
    }

    Refresh-ProcessPath
    return $true
}

function Use-SingleConfigBuildType {
    if ($env:CMAKE_GENERATOR) {
        return $env:CMAKE_GENERATOR -notmatch '(?i)visual studio|xcode|ninja multi-config'
    }
    return $false
}

Show-NoqeriBanner
Refresh-ProcessPath
$missing = @(Get-MissingPrerequisites)

if ($missing.Count -gt 0) {
    Write-Host 'Noqeri needs the following missing build prerequisite(s):'
    foreach ($item in $missing) { Write-Host "  - $item" }

    if ($SkipInstall) {
        Write-Warning 'Automatic prerequisite installation was disabled. The build is being skipped.'
        exit 2
    }

    $install = $false
    if ($Yes -or $env:NOQERI_INSTALL_MISSING -eq '1') {
        $install = $true
    } else {
        $answer = Read-Host 'Do you want to install all missing prerequisites now? [Y/N]'
        $install = $answer -match '^(?i:y|yes)$'
    }

    if ($install) {
        if (-not (Install-MissingPrerequisites -Missing $missing)) { exit 2 }
    } else {
        Write-Host 'No installation was performed.'
        Write-Host 'Prerequisite detection completed; the build is skipped until the missing tools are installed.'
        exit 2
    }
}

$remaining = @(Get-MissingPrerequisites)
if ($remaining.Count -gt 0) {
    Write-Warning ('Still missing after installation: ' + ($remaining -join ', '))
    Write-Host 'If Visual Studio Build Tools has just finished installing, open a new PowerShell window and rerun .\scripts\build.ps1.'
    exit 2
}

Push-Location $RepoRoot
try {
    Write-Host "building Noqeri bootstrap ($BuildType)"
    $configureArgs = @('-S', '.', '-B', $BuildDir)
    if ($env:CMAKE_GENERATOR) { $configureArgs += @('-G', $env:CMAKE_GENERATOR) }
    if (Use-SingleConfigBuildType) { $configureArgs += "-DCMAKE_BUILD_TYPE=$BuildType" }
    cmake @configureArgs
    cmake --build $BuildDir --config $BuildType --parallel

    $candidates = @(
        (Join-Path $BuildDir "$BuildType\noqeri.exe"),
        (Join-Path $BuildDir 'noqeri.exe')
    )
    $compiler = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $compiler) {
        throw "build completed but noqeri.exe was not found in $BuildDir"
    }

    $elapsed = Format-Elapsed ((Get-Date) - $StartedAt)
    Write-Host ''
    Show-NoqeriBanner
    Write-Host 'Noqeri build worked.'
    Write-Host "Time taken: $elapsed"
    Write-Host "Compiler: $compiler"
    Write-Host ''
    Write-Host 'Version:'
    & $compiler --version
    Show-NextSteps -Compiler $compiler
} finally {
    Pop-Location
}
