[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
# This generates the actual decoder helpers and also checks sound/texture
# regressions, including scene cleanup shared with the geometry cache.
& (Join-Path $PSScriptRoot 'test_audio_xdk.ps1')
$out = Join-Path $root 'build-x360/host-tests/audio-xdk'
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $install) { throw 'MSVC host compiler not found.' }
$vcvars = Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$batch = @"
@echo off
call "$vcvars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /I"$root\src\xdk" /I"$root\upstream\melee-pc\src\sdk_include" /Fo"$out\test_vertex_cache.obj" /Fe"$out\test_vertex_cache.exe" "$root\tests\host\test_vertex_cache.cpp"
if errorlevel 1 exit /b 1
"$out\test_vertex_cache.exe"
exit /b %errorlevel%
"@
$batchPath = Join-Path $out 'test-vertex-cache.bat'
Set-Content $batchPath -Encoding ASCII $batch
& cmd.exe /d /c $batchPath
if ($LASTEXITCODE -ne 0) { throw 'Decoded geometry cache regression failed.' }
