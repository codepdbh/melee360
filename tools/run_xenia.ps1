[CmdletBinding()]
param(
    [string] $XeniaPath = '',
    [string] $XexPath = '',
    [switch] $DisableKeyboard
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

$xeniaDirectory = Split-Path -Parent $XeniaPath
$xeniaConfig = Join-Path $xeniaDirectory 'xenia-canary.config.toml'
if (-not $DisableKeyboard -and (Test-Path -LiteralPath $xeniaConfig)) {
    $configText = Get-Content -Raw -LiteralPath $xeniaConfig
    $updatedConfig = $configText -replace '(?m)^keyboard_mode\s*=\s*\d+', 'keyboard_mode = 1'
    if ($updatedConfig -ne $configText) {
        Set-Content -LiteralPath $xeniaConfig -Value $updatedConfig -Encoding UTF8
        Write-Host '[M360][XENIA] keyboard controller enabled for user 0'
    }
}

Write-Host "[M360][XENIA] emulator: $XeniaPath"
Write-Host "[M360][XENIA] executable: $XexPath"
Start-Process -FilePath $XeniaPath -ArgumentList @($XexPath)
