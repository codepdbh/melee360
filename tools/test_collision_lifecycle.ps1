[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $install) { throw 'MSVC host compiler not found.' }
$vcvars = Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$out = Join-Path $root 'build-x360/host-tests/collision-lifecycle'
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
    @{ Path='upstream/melee-pc/src/melee/mp/mpcoll.c'; Functions=@('void mpColl_80041EE4(', 'void mpCopyCollData(') }
)) {
    $source = Get-Content -Raw (Join-Path $root $group.Path)
    foreach ($signature in $group.Functions) { $extracted += (Get-Function $source $signature) + "`r`n" }
}
Set-Content (Join-Path $out 'collision_lifecycle_original.h') -Encoding ASCII $extracted
$batch = @"
@echo off
call "$vcvars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /std:c++17 /W4 /WX /I"$out" /Fo"$out\test_collision_lifecycle.obj" /Fe"$out\test_collision_lifecycle.exe" "$root\tests\host\test_collision_lifecycle.cpp"
if errorlevel 1 exit /b 1
"$out\test_collision_lifecycle.exe"
exit /b %errorlevel%
"@
$batchPath = Join-Path $out 'test.bat'
Set-Content $batchPath -Encoding ASCII $batch
& cmd.exe /d /c $batchPath
if ($LASTEXITCODE -ne 0) { throw 'Original collision lifecycle regression failed.' }
