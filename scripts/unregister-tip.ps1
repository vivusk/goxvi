# Unregister goxvi-tsf.dll (admin; self-elevates with a UAC prompt).
# Usage: .\scripts\unregister-tip.ps1 [-Config Debug|Release]
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
        Write-Host "Unregistered OK."
    } else {
        Write-Host "Unregistration failed (exit $($p.ExitCode))."
    }
    exit $p.ExitCode
}

$repoRoot = Split-Path -Parent $PSScriptRoot
# Match register-tip.ps1: x86 on 32-bit Windows, x64 on 64-bit Windows.
$arch = if ([Environment]::Is64BitOperatingSystem) { "x64" } else { "x86" }
$dll = Join-Path $repoRoot "build\$arch\tsf\$Config\goxvi-tsf.dll"
if (-not (Test-Path $dll)) { throw "Not found: $dll" }

# regsvr32 is a GUI-subsystem exe: '&' would not wait and leaves
# $LASTEXITCODE stale. Start-Process -Wait gets the real exit code.
$reg = Start-Process -FilePath "regsvr32.exe" -ArgumentList "/s", "/u", "`"$dll`"" -Wait -PassThru
if ($reg.ExitCode -ne 0) { throw "regsvr32 /u failed (exit $($reg.ExitCode))" }
Write-Host "Unregistered: $dll"
