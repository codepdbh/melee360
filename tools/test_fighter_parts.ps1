[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $install) { throw 'MSVC host compiler not found.' }
$vcvars = Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$out = Join-Path $root 'build-x360/host-tests/fighter-parts'
New-Item -ItemType Directory -Force $out | Out-Null
$source = Get-Content -Raw (Join-Path $root 'src/xdk/fighter_glue_xdk.c')
$extracted = ''
foreach ($signature in @('u32 ftParts_8007506C(', 'static int SetupFighterParts(', 'void lb_80014498(',
    'static HSD_TObj* FindFighterTexture(', 'static int SetupCostumeTextures(',
    'void ftAnim_80070458(', 'void ftAnim_800704F0(', 'void ftAnim_800705E0(', 'void ftAnim_80070654(',
    'void ftCo_800A0098(', 'static void SetupCostumeVisibility(', 'static float FighterCollisionTop(',
    'static void SetupAnimationLengths(', 'static void AttachReservedJoint(',
    'static HSD_Joint* ReservedJointDescriptor(', 'bool ft_80084CE4(')) {
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
Set-Content -LiteralPath (Join-Path $out 'fighter_parts_original.h') -Encoding ASCII $extracted
$cleanupStart = $source.IndexOf('void ftParts_800755E8(')
$cleanupCursor = $source.IndexOf('{', $cleanupStart) + 1
$cleanupDepth = 1
while ($cleanupDepth -gt 0 -and $cleanupCursor -lt $source.Length) {
    if ($source[$cleanupCursor] -eq '{') { ++$cleanupDepth }
    if ($source[$cleanupCursor] -eq '}') { --$cleanupDepth }
    ++$cleanupCursor
}
if ($cleanupDepth) { throw 'Unbalanced reserved-part cleanup function.' }
Set-Content -LiteralPath (Join-Path $out 'reserved_cleanup_original.h') -Encoding ASCII $source.Substring($cleanupStart, $cleanupCursor - $cleanupStart)

$batch = @"
@echo off
call "$vcvars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /Fo"$out\test_fighter_parts.obj" /Fe"$out\test_fighter_parts.exe" "$root\tests\host\test_fighter_parts.cpp"
if errorlevel 1 exit /b 1
"$out\test_fighter_parts.exe"
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /Fo"$out\test_reserved_cleanup.obj" /Fe"$out\test_reserved_cleanup.exe" "$root\tests\host\test_reserved_cleanup.cpp"
if errorlevel 1 exit /b 1
"$out\test_reserved_cleanup.exe"
exit /b %errorlevel%
"@
$batchPath = Join-Path $out 'test.bat'
Set-Content -LiteralPath $batchPath -Encoding ASCII $batch
& cmd.exe /d /c $batchPath
if ($LASTEXITCODE -ne 0) { throw 'Fighter parts regression failed.' }
