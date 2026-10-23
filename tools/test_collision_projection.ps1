[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$out=Join-Path $root 'build-x360/host-tests/collision-projection'
New-Item -ItemType Directory -Force $out | Out-Null
& python (Join-Path $PSScriptRoot 'generate_collision_projection.py') (Join-Path $out 'collision_projection.h')
if($LASTEXITCODE -ne 0) { throw 'Projection generation failed.' }
$generator=@'
import sys
from pathlib import Path
sys.path.insert(0,'tools')
from generate_menu_sources import functions
from generate_collision_projection import NAMES
source=functions(Path('upstream/melee-pc/src/melee/mp/mplib.c').read_text(encoding='utf-8'))
Path(sys.argv[1]).write_text('\n\n'.join(source[n][1].replace(n+'(', 'Original_'+n+'(',1) for n in NAMES),encoding='ascii')
native=functions(Path('src/xdk/fighter_glue_xdk.c').read_text(encoding='utf-8'))
Path(sys.argv[1]).with_name('ground_step_native.h').write_text(native['GroundStep'][1],encoding='ascii')
Path(sys.argv[1]).with_name('ground_edge_native.h').write_text(native['GroundStepEdge'][1],encoding='ascii')
coll=functions(Path('upstream/melee-pc/src/melee/mp/mpcoll.c').read_text(encoding='utf-8'))
Path(sys.argv[1]).with_name('edge_original.h').write_text('\n\n'.join(coll[n][1] for n in ('mpColl_8004A45C_Floor','mpColl_8004A678_Floor')),encoding='ascii')
'@
Push-Location $root
try { $generator | python - (Join-Path $out 'projection_reference.h') } finally { Pop-Location }
if($LASTEXITCODE -ne 0) { throw 'Original reference extraction failed.' }
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $install) { throw 'MSVC host compiler not found.' }
$vcvars=Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$batch=@"
@echo off
call "$vcvars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /I"$root\src\xdk" /Fo"$out\projection.obj" /Fe"$out\projection.exe" "$root\tests\host\test_collision_projection.cpp"
if errorlevel 1 exit /b 1
"$out\projection.exe"
exit /b %errorlevel%
"@
Set-Content (Join-Path $out 'test.bat') -Encoding ASCII $batch
& cmd.exe /d /c (Join-Path $out 'test.bat')
if($LASTEXITCODE -ne 0) { throw 'Original projection regression failed.' }
