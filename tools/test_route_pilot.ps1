[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $install) { throw 'MSVC host compiler not found.' }
$vcvars = Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$out = Join-Path $root 'build-x360/host-tests/route-pilot'
New-Item -ItemType Directory -Force $out | Out-Null
$source = Get-Content -Raw (Join-Path $root 'src/xdk/melee_pad_xdk.cpp')
$extracted = ''
foreach ($signature in @('static void PilotSurfaceSpan(', 'static int PilotCeilingDetour(', 'static int PilotMazeSteer(', 'static bool PilotMazeCanJump(')) {
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
    $extracted += $source.Substring($start, $cursor - $start) + "`r`n"
}
Set-Content (Join-Path $out 'route_pilot_original.h') -Encoding ASCII $extracted
$batch = @"
@echo off
call "$vcvars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /I"$root\src\xdk" /Fo"$out\test_route_pilot.obj" /Fe"$out\test_route_pilot.exe" "$root\tests\host\test_route_pilot.cpp"
if errorlevel 1 exit /b 1
"$out\test_route_pilot.exe"
exit /b %errorlevel%
"@
$batchPath = Join-Path $out 'test.bat'
Set-Content $batchPath -Encoding ASCII $batch
& cmd.exe /d /c $batchPath
if ($LASTEXITCODE -ne 0) { throw 'Diagnostic route navigation regression failed.' }
