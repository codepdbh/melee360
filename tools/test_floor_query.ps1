[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$out=Join-Path $root 'build-x360/host-tests/floor-query'
New-Item -ItemType Directory -Force $out | Out-Null
& python (Join-Path $PSScriptRoot 'generate_collision_projection.py') (Join-Path $out 'collision_projection.h')
if($LASTEXITCODE -ne 0) { throw 'Projection generation failed.' }
& python (Join-Path $PSScriptRoot 'generate_floor_query.py') (Join-Path $out 'collision_floor_query.h')
if($LASTEXITCODE -ne 0) { throw 'Floor query generation failed.' }
$generator=@'
import sys
from pathlib import Path
sys.path.insert(0,'tools')
from generate_menu_sources import functions
f=functions(Path('upstream/melee-pc/src/melee/mp/mplib.c').read_text(encoding='utf-8'))
names=('mpLib_8004ED5C','mpLineIntersection','mpLineIntersectionH','mpCheckFloor')
text='\n\n'.join(f[n][1] for n in names)
for n in names:text=text.replace(n+'(', 'Original_'+n+'(')
out=Path(sys.argv[1]);out.write_text(text,encoding='ascii')
native=functions(Path('src/xdk/fighter_glue_xdk.c').read_text(encoding='utf-8'))
coll=functions(Path('upstream/melee-pc/src/melee/mp/mpcoll.c').read_text(encoding='utf-8'))
out.with_name('floor_landing_native.h').write_text(native['SetFloorColl'][1]+'\n'+coll['mpColl_80044838_Floor'][1]+'\n'+native['LandBetweenColl'][1],encoding='ascii')
'@
Push-Location $root
try { $generator | python - (Join-Path $out 'floor_query_reference.h') } finally { Pop-Location }
if($LASTEXITCODE -ne 0) { throw 'Reference extraction failed.' }
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $install) { throw 'MSVC host compiler not found.' }
$vcvars=Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$batch=@"
@echo off
call "$vcvars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /I"$root\src\xdk" /Fo"$out\floor.obj" /Fe"$out\floor.exe" "$root\tests\host\test_floor_query.cpp"
if errorlevel 1 exit /b 1
"$out\floor.exe"
exit /b %errorlevel%
"@
Set-Content (Join-Path $out 'test.bat') -Encoding ASCII $batch
& cmd.exe /d /c (Join-Path $out 'test.bat')
if($LASTEXITCODE -ne 0) { throw 'Original floor query regression failed.' }
