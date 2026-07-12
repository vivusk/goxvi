# Register goxvi-tsf.dll as a TSF text input processor (writes HKLM - admin).
# Self-elevates with an explicit UAC prompt when not already elevated.
# Usage: .\scripts\register-tip.ps1 [-Config Debug|Release]
# Note (L3): the registry stores the ABSOLUTE path of ONE config. Switching
# Debug<->Release requires re-running this script with the other -Config.
# NOTE: keep this file pure ASCII - Windows PowerShell 5.1 reads BOM-less
# .ps1 files as ANSI and mangles multi-byte characters into parse errors.
param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"

$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "Not elevated - requesting admin via UAC prompt..."
    $p = Start-Process -FilePath "powershell.exe" `
        -ArgumentList "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "`"$PSCommandPath`"", "-Config", $Config `
        -Verb RunAs -Wait -PassThru
    if ($p.ExitCode -eq 0) {
        Write-Host "Registered OK. Select 'Goxvi Telex' via Win+Space."
    } else {
        Write-Host "Registration failed (exit $($p.ExitCode))."
    }
    exit $p.ExitCode
}

$repoRoot = Split-Path -Parent $PSScriptRoot
# Genuine 32-bit Windows loads only the x86 DLL; 64-bit Windows uses x64.
# regsvr32.exe in System32 is native-bitness on each OS, so DllRegisterServer's
# HKCR\CLSID write lands in the correct hive automatically (no Wow6432Node
# handling needed here - that only matters when serving 32-bit apps on 64-bit OS).
$arch = if ([Environment]::Is64BitOperatingSystem) { "x64" } else { "x86" }
$dll = Join-Path $repoRoot "build\$arch\tsf\$Config\goxvi-tsf.dll"
if (-not (Test-Path $dll)) {
    throw "Not found: $dll - build first (scripts\build.ps1 -Config $Config, x86: cmake --preset x86 && cmake --build --preset x86-release)."
}

# regsvr32 is a GUI-subsystem exe: '&' would not wait and leaves
# $LASTEXITCODE stale. Start-Process -Wait gets the real exit code.
$reg = Start-Process -FilePath "regsvr32.exe" -ArgumentList "/s", "`"$dll`"" -Wait -PassThru
if ($reg.ExitCode -ne 0) { throw "regsvr32 failed (exit $($reg.ExitCode))" }
Write-Host "Registered: $dll"
Write-Host "Select 'Goxvi Telex' via Win+Space (Settings > Time & language > Language if missing)."
