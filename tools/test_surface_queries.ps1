[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$out=Join-Path $root 'build-x360/host-tests/surface-queries'
New-Item -ItemType Directory -Force $out | Out-Null
& python (Join-Path $PSScriptRoot 'generate_collision_projection.py') (Join-Path $out 'collision_projection.h')
if($LASTEXITCODE -ne 0) { throw 'Projection generation failed.' }
& python (Join-Path $PSScriptRoot 'generate_surface_queries.py') (Join-Path $out 'collision_surface_queries.h')
if($LASTEXITCODE -ne 0) { throw 'Surface query generation failed.' }
$generator=@'
import sys
from pathlib import Path
sys.path.insert(0,'tools')
from generate_menu_sources import functions
f=functions(Path('upstream/melee-pc/src/melee/mp/mplib.c').read_text(encoding='utf-8'))
names=('mpLib_8004ED5C','mpLineIntersection','mpLineIntersectionH','mpLineIntersectionV','mpCheckCeiling','mpCheckLeftWall','mpCheckRightWall')
text='\n\n'.join(f[n][1] for n in names)
for n in names:text=text.replace(n+'(', 'Original_'+n+'(')
out=Path(sys.argv[1]);out.write_text(text,encoding='ascii')
'@
Push-Location $root
try { $generator | python - (Join-Path $out 'surface_queries_reference.h') } finally { Pop-Location }
if($LASTEXITCODE -ne 0) { throw 'Reference extraction failed.' }
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $install) { throw 'MSVC host compiler not found.' }
$vcvars=Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$batch=@"
@echo off
call "$vcvars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /I"$root\src\xdk" /Fo"$out\surfaces.obj" /Fe"$out\surfaces.exe" "$root\tests\host\test_surface_queries.cpp"
if errorlevel 1 exit /b 1
"$out\surfaces.exe"
exit /b %errorlevel%
"@
Set-Content (Join-Path $out 'test.bat') -Encoding ASCII $batch
& cmd.exe /d /c (Join-Path $out 'test.bat')
if($LASTEXITCODE -ne 0) { throw 'Original surface queries regression failed.' }
