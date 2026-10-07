[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $install) { throw 'MSVC host compiler not found.' }
$vcvars = Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$out = Join-Path $root 'build-x360/host-tests/fighter-camera'
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
$extracted = ''
foreach ($group in @(
    @{ Path='src/xdk/fighter_unported_xdk.c'; Functions=@('float Stage_GetCamFixedZoom(', 'void Stage_UnkSetVec3TCam_Offset(') },
    @{ Path='upstream/melee-pc/src/melee/ft/ftcamera.c'; Functions=@('void ftCamera_80076018(', 'void ftCamera_80076064(', 'void ftCamera_UpdateCameraBox(', 'void ftCamera_800762F4(', 'void ftCamera_80076320(') },
    @{ Path='upstream/melee-pc/src/melee/ft/ftcommon.c'; Functions=@('float ftCommon_GetModelScale(') },
    @{ Path='upstream/melee-pc/src/melee/ft/fighter.c'; Functions=@('void Fighter_UnkCallCameraCallback_8006D9EC(', 'void Fighter_UpdateModelScale(') },
    @{ Path='src/xdk/fighter_glue_xdk.c'; Functions=@('void M360_FighterSetEncounter(') }
)) {
    $source = Get-Content -Raw (Join-Path $root $group.Path)
    foreach ($signature in $group.Functions) {
        $extracted += (Get-Function $source $signature) + "`r`n"
    }
}
Set-Content (Join-Path $out 'fighter_camera_original.h') -Encoding ASCII $extracted
$batch = @"
@echo off
call "$vcvars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /I"$root\src\xdk" /Fo"$out\test_fighter_camera.obj" /Fe"$out\test_fighter_camera.exe" "$root\tests\host\test_fighter_camera.cpp"
if errorlevel 1 exit /b 1
"$out\test_fighter_camera.exe"
exit /b %errorlevel%
"@
$batchPath = Join-Path $out 'test.bat'
Set-Content $batchPath -Encoding ASCII $batch
& cmd.exe /d /c $batchPath
if ($LASTEXITCODE -ne 0) { throw 'Original fighter camera regression failed.' }
