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
    $keyboardBindings = @{
        keybind_a = 'J'
        keybind_b = 'K'
        keybind_x = 'L'
        keybind_y = 'I'
        keybind_start = '0x0D'
        keybind_back = '0x1B'
        keybind_left_thumb_up = 'W'
        keybind_left_thumb_down = 'S'
        keybind_left_thumb_left = 'A'
        keybind_left_thumb_right = 'D'
        keybind_left_shoulder = '1'
        keybind_right_shoulder = '3'
        keybind_left_trigger = 'Q'
        keybind_right_trigger = 'E'
    }
    foreach ($binding in $keyboardBindings.GetEnumerator()) {
        $pattern = '(?m)^' + [regex]::Escape($binding.Key) + '\s*=\s*"[^"]*"'
        $replacement = $binding.Key + ' = "' + $binding.Value + '"'
        $updatedConfig = [regex]::Replace($updatedConfig, $pattern, $replacement)
    }
    if ($updatedConfig -ne $configText) {
        Set-Content -LiteralPath $xeniaConfig -Value $updatedConfig -Encoding UTF8
        Write-Host '[M360][XENIA] keyboard controller enabled: WASD move, J/K/L/I face buttons, Enter start'
    }
}

Write-Host "[M360][XENIA] emulator: $XeniaPath"
Write-Host "[M360][XENIA] executable: $XexPath"
Start-Process -FilePath $XeniaPath -ArgumentList @('--allow_game_relative_writes=true', $XexPath)
