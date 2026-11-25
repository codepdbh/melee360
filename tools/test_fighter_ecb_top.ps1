[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$out=Join-Path $root 'build-x360/host-tests/fighter-ecb-top'
New-Item -ItemType Directory -Force $out | Out-Null
$generator=@'
import sys
from pathlib import Path
sys.path.insert(0,'tools')
from generate_menu_sources import functions
src=functions(Path('upstream/melee-pc/src/melee/mp/mpcoll.c').read_text(encoding='utf-8'))
names=('clamp_above','clamp_below','mpColl_80042384','update_min_max','mpColl_LoadECB_JObj')
out=Path(sys.argv[1]);out.write_text('\n'.join(src[n][1] for n in names),encoding='ascii')
native=functions(Path('src/xdk/fighter_glue_xdk.c').read_text(encoding='utf-8'))
names=('FighterCollisionTop','AirCeilings','AirWalls','FindLedge')
out.with_name('fighter_ecb_native.h').write_text('\n'.join(native[name][1] for name in names),encoding='ascii')
'@
Push-Location $root
try { $generator | python - (Join-Path $out 'fighter_ecb_original.h') } finally { Pop-Location }
if($LASTEXITCODE -ne 0) { throw 'Original ECB extraction failed.' }
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $install) { throw 'MSVC host compiler not found.' }
$vcvars=Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$batch=@"
@echo off
call "$vcvars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /std:c++17 /W4 /WX /I"$out" /I"$root\src\xdk" /Fo"$out\test.obj" /Fe"$out\test.exe" "$root\tests\host\test_fighter_ecb_top.cpp"
if errorlevel 1 exit /b 1
"$out\test.exe"
exit /b %errorlevel%
"@
Set-Content (Join-Path $out 'test.bat') -Encoding ASCII $batch
& cmd.exe /d /c (Join-Path $out 'test.bat')
if($LASTEXITCODE -ne 0) { throw 'Fighter ECB sampling regression failed.' }
