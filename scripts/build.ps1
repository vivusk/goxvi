# Build goxvi. Usage: .\scripts\build.ps1 [-Config Debug|Release] [-Arch x64|x86|All] [-Test]
#   -Arch      x64, x86, or All (default: All - builds both so 32-bit Win10 stays in sync).
#   -Test      run ctest for each built arch (x86 tests run via WoW64 on 64-bit host).
# The native Settings app (Goxvi.exe, target goxvi-settings) builds automatically
# as part of the CMake build - no separate dotnet step (replaced the WinUI 3 app).
# NOTE: keep this file pure ASCII - Windows PowerShell 5.1 reads BOM-less .ps1
# files as ANSI and mangles multi-byte characters into parse errors.
param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",
    [ValidateSet("x64", "x86", "All")]
    [string]$Arch = "All",
    [switch]$Test
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

# Prefer cmake on PATH, fall back to the VS 2022 bundled one.
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmake) {
    $cmake = $cmake.Source
} else {
    $vsCmake = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    if (-not (Test-Path $vsCmake)) { throw "cmake not found (PATH or VS 2022 Community)" }
    $cmake = $vsCmake
}
$ctest = Join-Path (Split-Path -Parent $cmake) "ctest.exe"

$arches = if ($Arch -eq "All") { @("x64", "x86") } else { @($Arch) }
$cfgLower = $Config.ToLower()

Push-Location $repoRoot
try {
    foreach ($a in $arches) {
        Write-Host "=== Building $a ($Config) ===" -ForegroundColor Cyan
        $preset = "$a-$cfgLower"

        & $cmake --preset $a
        if ($LASTEXITCODE -ne 0) { throw "configure failed ($a)" }

        & $cmake --build --preset $preset
        if ($LASTEXITCODE -ne 0) { throw "build failed ($a)" }

        if ($Test) {
            & $ctest --preset $preset
            if ($LASTEXITCODE -ne 0) { throw "tests failed ($a)" }
        }
    }
} finally {
    Pop-Location
}

Write-Host "Done: $($arches -join ', ') ($Config) (incl. native Settings Goxvi.exe)." -ForegroundColor Green
