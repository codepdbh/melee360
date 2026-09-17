[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$sdkKey = 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Xbox\2.0\SDK'
$sdk = Get-ItemProperty -LiteralPath $sdkKey -ErrorAction SilentlyContinue
$root = $env:XEDK
if (-not $root -and $sdk) {
    $root = $sdk.InstallPath
}

Write-Host '[M360][XDK] Xbox 360 SDK check'
if (-not $root -or -not (Test-Path -LiteralPath $root)) {
    Write-Host '[M360][XDK][FAIL] XEDK/SDK installation was not found.'
    exit 1
}

Write-Host "[M360][XDK] root: $root"
if ($sdk) {
    Write-Host "[M360][XDK] version: $($sdk.InstalledVersion)"
    Write-Host "[M360][XDK] install type: $($sdk.InstallType)"
}

function Find-XdkFile([string] $Name) {
    Get-ChildItem -LiteralPath $root -Recurse -File -Filter $Name `
        -ErrorAction SilentlyContinue | Select-Object -First 1
}

$imagexex = Find-XdkFile 'imagexex.exe'
$compiler = Find-XdkFile 'cl.exe'
$header = Find-XdkFile 'xtl.h'
$kernelLib = Find-XdkFile 'xboxkrnl.lib'

$checks = @(
    @{ Name = 'imagexex'; Value = $imagexex },
    @{ Name = 'PowerPC compiler'; Value = $compiler },
    @{ Name = 'XTL headers'; Value = $header },
    @{ Name = 'Xbox kernel import library'; Value = $kernelLib }
)

$missing = 0
foreach ($check in $checks) {
    if ($check.Value) {
        Write-Host "[M360][XDK][OK] $($check.Name): $($check.Value.FullName)"
    } else {
        Write-Host "[M360][XDK][MISSING] $($check.Name)"
        $missing++
    }
}

if ($missing -ne 0) {
    Write-Host '[M360][XDK][INCOMPLETE] Install the complete Xbox 360 C/C++ development tools.'
    Write-Host '[M360][XDK] A minimum/tools-only installation cannot compile the PE input required by imagexex.'
    exit 2
}

Write-Host '[M360][XDK][READY] Complete XEX toolchain detected.'
