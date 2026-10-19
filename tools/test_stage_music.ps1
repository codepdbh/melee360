[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$out=Join-Path $root 'build-x360/host-tests/stage-music'
New-Item -ItemType Directory -Force $out | Out-Null
& python (Join-Path $PSScriptRoot 'generate_stage_music_sources.py') (Join-Path $out 'stage_music_original.h')
if($LASTEXITCODE -ne 0) { throw 'Music source extraction failed.' }
& python (Join-Path $PSScriptRoot 'audit_stage_music.py') --output (Join-Path $out 'audit.json') --selection-header (Join-Path $out 'stage_music_rows.h')
if($LASTEXITCODE -ne 0) { throw 'Disc music audit failed.' }
$native=Get-Content -Raw (Join-Path $root 'src/xdk/stage_music_xdk.c')
$native=[regex]::Replace($native,'(?m)^#include (?!"stage_music_original.h")[^\n]*\n','')
Set-Content -Encoding ASCII (Join-Path $out 'stage_music_native.h') $native
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $install) { throw 'MSVC host compiler not found.' }
$vcvars=Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$batch=@"
@echo off
call "$vcvars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /Fo"$out/test_stage_music.obj" /Fe"$out/test_stage_music.exe" "$root/tests/host/test_stage_music.cpp"
if errorlevel 1 exit /b 1
"$out/test_stage_music.exe"
exit /b %errorlevel%
"@
$path=Join-Path $out 'test.bat'
Set-Content -Encoding ASCII $path $batch
& cmd.exe /d /c $path
if($LASTEXITCODE -ne 0) { throw 'Original stage music regression failed.' }
