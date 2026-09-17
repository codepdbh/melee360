[CmdletBinding()]
param(
    [string] $XeniaPath = '',
    [string] $XexPath = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $XeniaPath) {
    $XeniaPath = Join-Path $root 'build-x360\xenia-canary\xenia_canary.exe'
}
if (-not $XexPath) {
    $XexPath = Join-Path $root 'dist\default.xex'
}

if (-not (Test-Path -LiteralPath $XeniaPath)) {
    throw "Xenia Canary was not found: $XeniaPath"
}
if (-not (Test-Path -LiteralPath $XexPath)) {
    throw "Build the XEX first with tools/build_xex.ps1: $XexPath"
}

Write-Host "[M360][XENIA] emulator: $XeniaPath"
Write-Host "[M360][XENIA] executable: $XexPath"
Start-Process -FilePath $XeniaPath -ArgumentList @($XexPath)
