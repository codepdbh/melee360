[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$out=Join-Path $root 'build-x360/host-tests/campaign'
New-Item -ItemType Directory -Force $out | Out-Null
& python (Join-Path $PSScriptRoot 'generate_campaign_sources.py') (Join-Path $out 'campaign_original.h')
if($LASTEXITCODE -ne 0) { throw 'Campaign extraction failed.' }
# C permits this void* conversion; the C++ host harness needs its explicit cast.
$originalPath=Join-Path $out 'campaign_original.h'
$original=Get-Content -Raw $originalPath
if(-not $original.Contains('out = arg0;')) { throw 'Original ally pointer assignment changed.' }
Set-Content -Encoding ASCII $originalPath ($original.Replace('out = arg0;','out = (RegClearCharEntry*)arg0;'))
& (Join-Path $PSScriptRoot 'generate_classic_matchups.ps1') -OutputPath (Join-Path $out 'melee_classic_matchups.h')
& python (Join-Path $PSScriptRoot 'generate_adventure_matchups.py') (Join-Path $out 'melee_adventure_matchups.h')
if($LASTEXITCODE -ne 0) { throw 'Adventure metadata extraction failed.' }
$native=Get-Content -Raw (Join-Path $root 'src/xdk/campaign_xdk.c')
$header=Get-Content -Raw (Join-Path $root 'src/xdk/campaign_xdk.h')
Set-Content -Encoding ASCII (Join-Path $out 'campaign_native.h') ([regex]::Replace($header+"`n"+$native,'(?m)^#include[^\n]*\n',''))
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $install) { throw 'MSVC host compiler not found.' }
$vcvars=Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$batch=@"
@echo off
call "$vcvars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /wd4505 /WX /I"$out" /Fo"$out/test_campaign.obj" /Fe"$out/test_campaign.exe" "$root/tests/host/test_campaign.cpp"
if errorlevel 1 exit /b 1
"$out/test_campaign.exe"
exit /b %errorlevel%
"@
$path=Join-Path $out 'test.bat'
Set-Content -Encoding ASCII $path $batch
& cmd.exe /d /c $path
if($LASTEXITCODE -ne 0) { throw 'Original campaign regression failed.' }
