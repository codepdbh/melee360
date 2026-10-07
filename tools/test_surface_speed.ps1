[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $install) { throw 'MSVC host compiler not found.' }
$vcvars = Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$out = Join-Path $root 'build-x360/host-tests/surface-speed'
New-Item -ItemType Directory -Force $out | Out-Null
function Get-Function([string]$source, [string]$signature) {
    $start = $source.IndexOf($signature)
    if ($start -lt 0) { throw "Missing function: $signature" }
    $cursor = $source.IndexOf('{', $start) + 1
    $depth = 1
    while ($depth -gt 0 -and $cursor -lt $source.Length) {
        if ($source[$cursor] -eq '{') { ++$depth }
        if ($source[$cursor] -eq '}') { --$depth }
        ++$cursor
    }
    if ($depth) { throw "Unbalanced function: $signature" }
    return $source.Substring($start, $cursor - $start)
}
$source = Get-Content -Raw (Join-Path $root 'src/xdk/fighter_glue_xdk.c')
$extracted = ''
foreach ($signature in @('bool mpLib_80054ED8(', 'static void mpRemap2d(', 'bool mpGetSpeed(',
    'bool mpCollGetSpeedFloor(', 'bool mpCollGetSpeedCeiling(', 'bool mpCollGetSpeedLeftWall(', 'bool mpCollGetSpeedRightWall(')) {
    $extracted += (Get-Function $source $signature) + "`r`n"
}
Set-Content (Join-Path $out 'surface_speed_original.h') -Encoding ASCII $extracted
$upstream = Get-Content -Raw (Join-Path $root 'upstream/melee-pc/src/melee/mp/mplib.c')
$reference = '#define ABS(x) ((x) < 0 ? -(x) : (x))' + "`r`n" + (Get-Function $upstream 'static void mpRemap2d(').Replace('mpRemap2d(', 'OriginalRemap(')
Set-Content (Join-Path $out 'surface_remap_reference.h') -Encoding ASCII $reference
$batch = @"
@echo off
call "$vcvars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /I"$root\src\xdk" /Fo"$out\test_surface_speed.obj" /Fe"$out\test_surface_speed.exe" "$root\tests\host\test_surface_speed.cpp"
if errorlevel 1 exit /b 1
"$out\test_surface_speed.exe"
exit /b %errorlevel%
"@
$batchPath = Join-Path $out 'test.bat'
Set-Content $batchPath -Encoding ASCII $batch
& cmd.exe /d /c $batchPath
if ($LASTEXITCODE -ne 0) { throw 'Original surface speed regression failed.' }
