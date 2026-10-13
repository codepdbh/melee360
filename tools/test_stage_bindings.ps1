[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $install) { throw 'MSVC host compiler not found.' }
$vcvars = Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$out = Join-Path $root 'build-x360/host-tests/stage-bindings'
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
$scene = Get-Content -Raw (Join-Path $root 'src/xdk/match_scene_xdk.c')
if (-not $scene.Contains('BindMapColl(stage, id, root);')) { throw 'Stage binding must start at the scale wrapper.' }
if ($scene.Contains('M360_FighterFollowFloors(')) { throw 'Platform displacement is already applied by Fighter_procUpdate.' }
$extracted = ''
foreach ($signature in @('static HSD_JObj* FindCollJoint(', 'static void AddCollBind(', 'static void BindMapColl(')) {
    $extracted += (Get-Function $scene $signature) + "`r`n"
}
Set-Content (Join-Path $out 'stage_bindings_native.h') -Encoding ASCII $extracted
$original = Get-Content -Raw (Join-Path $root 'upstream/melee-pc/src/melee/mp/mplib.c')
Set-Content (Join-Path $out 'stage_bindings_reference.h') -Encoding ASCII ((Get-Function $original 'void mpLib_800552B0(').Replace('mpLib_800552B0(', 'OriginalBind('))
$batch = @"
@echo off
call "$vcvars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /Fo"$out\test_stage_bindings.obj" /Fe"$out\test_stage_bindings.exe" "$root\tests\host\test_stage_bindings.cpp"
if errorlevel 1 exit /b 1
"$out\test_stage_bindings.exe"
exit /b %errorlevel%
"@
$batchPath = Join-Path $out 'test.bat'
Set-Content $batchPath -Encoding ASCII $batch
& cmd.exe /d /c $batchPath
if ($LASTEXITCODE -ne 0) { throw 'Stage collision binding regression failed.' }
