[CmdletBinding()]
param(
    [string] $XeniaPath = '',
    [string] $XexPath = '',
    [string] $IsoPath = '',
    [switch] $DisableKeyboard,
    [switch] $Mute
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $XeniaPath) {
    $XeniaPath = Join-Path $root 'build-x360\xenia-canary\xenia_canary.exe'
}
if (-not $XexPath) {
    $XexPath = Join-Path $root 'dist\default.xex'
}

if (-not (Test-Path -LiteralPath $XeniaPath -PathType Leaf)) {
    throw "Xenia Canary was not found: $XeniaPath"
}
if (-not (Test-Path -LiteralPath $XexPath -PathType Leaf)) {
    throw "Build the XEX first with tools/build_xex.ps1: $XexPath"
}

$XeniaPath = (Resolve-Path -LiteralPath $XeniaPath).ProviderPath
$XexPath = (Resolve-Path -LiteralPath $XexPath).ProviderPath
$gameDirectory = Split-Path -Parent $XexPath
$localIso = Join-Path $gameDirectory 'melee.iso'
if ($IsoPath) {
    if (Test-Path -LiteralPath $localIso) {
        throw "A melee.iso already exists beside the XEX. Launch without -IsoPath to use it: $localIso"
    }
    if (-not (Test-Path -LiteralPath $IsoPath -PathType Leaf)) {
        throw "ISO file not found: $IsoPath"
    }
    $sourceIso = (Resolve-Path -LiteralPath $IsoPath).ProviderPath
    New-Item -ItemType HardLink -Path $localIso -Target $sourceIso | Out-Null
    Write-Host "[M360][XENIA] linked ISO as $localIso"
} elseif (-not (Test-Path -LiteralPath $localIso -PathType Leaf)) {
    throw "Place melee.iso beside the XEX, or specify -IsoPath to link a local ISO on the same volume: $localIso"
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
Write-Host "[M360][XENIA] audio enabled: $(-not $Mute.IsPresent)"
Start-Process -FilePath $XeniaPath -WorkingDirectory $xeniaDirectory -ArgumentList @('--allow_game_relative_writes=true', '--apu=xaudio2', "--mute=$($Mute.IsPresent.ToString().ToLowerInvariant())", "`"$XexPath`"")
