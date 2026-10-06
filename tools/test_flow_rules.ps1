[CmdletBinding()]
param([string] $VcVars = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $VcVars) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $install = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $install) { throw 'MSVC host compiler not found; specify -VcVars.' }
    $VcVars = Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
}
$out = Join-Path $root 'build-x360/host-tests/flow-rules'
New-Item -ItemType Directory -Force $out | Out-Null
$source = Get-Content -Raw (Join-Path $root 'src/xdk/melee_flow_xdk.cpp')
$extracted = ''
foreach ($signature in @('void EnterState(', 'void UpdateMenu(', 'void UpdateMatch(', 'void LoadAutoConfig(')) {
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
# Only the platform types are replaced; the flow structure and enums are original.
Set-Content -LiteralPath (Join-Path $out 'xtl.h') -Encoding ASCII @'
#pragma once
struct LARGE_INTEGER { long long QuadPart; };
struct IDirect3DDevice9;
inline int QueryPerformanceCounter(LARGE_INTEGER* value) { value->QuadPart = 1; return 1; }
'@
Set-Content -LiteralPath (Join-Path $out 'flow_rules_original.h') -Encoding ASCII $extracted
$batch = @"
@echo off
call "$VcVars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /Fo"$out\test_flow_rules.obj" /Fe"$out\test_flow_rules.exe" "$root\tests\host\test_flow_rules.cpp"
if errorlevel 1 exit /b 1
"$out\test_flow_rules.exe"
exit /b %errorlevel%
"@
$batchPath = Join-Path $out 'test.bat'
Set-Content -LiteralPath $batchPath -Encoding ASCII $batch
& cmd.exe /d /c $batchPath
if ($LASTEXITCODE -ne 0) { throw 'Native flow rules regression failed.' }
