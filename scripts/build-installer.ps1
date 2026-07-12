# Build the Goxvi installer end-to-end: Release binaries (x64 + x86 TIP plus the
# native Settings app Goxvi.exe) then compile installer\goxvi.iss into
# dist\goxvi-setup-<ver>.exe.
# Usage: .\scripts\build-installer.ps1 [-SkipBuild]
#   -SkipBuild  reuse existing Release binaries, only re-run the Inno compiler.
# NOTE: keep this file pure ASCII (PowerShell 5.1 reads BOM-less .ps1 as ANSI).
param(
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

if (-not $SkipBuild) {
    # TIP x64 + x86 + native Settings (Goxvi.exe), all Release. The settings app
    # builds automatically as a CMake target - no separate dotnet step.
    & (Join-Path $PSScriptRoot "build.ps1") -Config Release -Arch All
    if ($LASTEXITCODE -ne 0) { throw "Release build failed" }
}

# Locate the Inno Setup compiler (winget installs to Program Files (x86)).
$iscc = @(
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) {
    throw "ISCC.exe not found. Install Inno Setup: winget install --id JRSoftware.InnoSetup"
}

$iss = Join-Path $repoRoot "installer\goxvi.iss"
& $iscc $iss
if ($LASTEXITCODE -ne 0) { throw "Inno Setup compile failed (exit $LASTEXITCODE)" }

$out = Get-ChildItem (Join-Path $repoRoot "dist") -Filter "goxvi-setup-*.exe" -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($out) { Write-Host "Installer: $($out.FullName)" -ForegroundColor Green }
