[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$out=Join-Path $root 'build-x360/host-tests/sis'
New-Item -ItemType Directory -Force $out | Out-Null
$source=Get-Content -Raw (Join-Path $root 'src/xdk/hsd_render_xdk.cpp')
$type=[regex]::Match($source,'(?s)struct TevStage \{.*?\n\};').Value
$clear=[regex]::Match($source,'(?s)void ClearStage\(TevStage\* s\)\s*\{.*?\n\}').Value
$start=$source.IndexOf('    TevStage* stage = reinterpret_cast<TevStage*>(ps + 12 * 4);')
$end=$source.IndexOf('    ps[48 * 4 + 0]', $start)
if(-not $type -or -not $clear -or $start -lt 0 -or $end -lt $start) { throw 'Native quad TEV setup not found.' }
$setup=$source.Substring($start,$end-$start)
Set-Content -Encoding ASCII (Join-Path $out 'sis_tev_original.h') ($type+"`n"+$clear+"`nvoid ConfigureQuadTev(float* ps,bool constantRgb) {`n"+$setup+"`n}`n")
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $install) { throw 'MSVC host compiler not found.' }
$vcvars=Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$batch=@"
@echo off
call "$vcvars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /I"$root/tests/host/sis_shim" /I"$root/src/xdk" /Fo"$out/test.obj" /Fe"$out/test.exe" "$root/tests/host/test_sis_native.cpp"
if errorlevel 1 exit /b 1
"$out/test.exe"
exit /b %errorlevel%
"@
Set-Content -Encoding ASCII (Join-Path $out 'test.bat') $batch
& cmd.exe /d /c (Join-Path $out 'test.bat')
if($LASTEXITCODE -ne 0) { throw 'SIS native boundary regression failed.' }
