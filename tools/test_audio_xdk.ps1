[CmdletBinding()]
param([string] $Iso = '', [string] $VcVars = '')
# Host regressions for native sound selection/submission and GX CMPR decoding.
# Uses the user's ISO and MSVC; no game data is written into the repository.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $Iso) { $Iso = Join-Path $root 'iso/MeleeUSAv1.02.iso' }
if (-not $VcVars) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $install = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $install) { throw 'MSVC host compiler not found; specify -VcVars.' }
    $VcVars = Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
}
if (-not (Test-Path -LiteralPath $Iso)) { throw "ISO not found: $Iso" }
$out = Join-Path $root 'build-x360/host-tests/audio-xdk'
New-Item -ItemType Directory -Force $out | Out-Null
# Execute upstream selectors on the host without substituting their logic.
function Get-SoundFunction([string] $Text, [string] $Signature) {
    $start = $Text.IndexOf($Signature)
    if ($start -lt 0) { throw "Missing original function: $Signature" }
    $cursor = $Text.IndexOf('{', $start) + 1
    $depth = 1
    while ($depth -gt 0 -and $cursor -lt $Text.Length) {
        if ($Text[$cursor] -eq '{') { ++$depth }
        if ($Text[$cursor] -eq '}') { --$depth }
        ++$cursor
    }
    if ($depth -ne 0) { throw "Unbalanced source: $Signature" }
    $Text.Substring($start, $cursor - $start)
}
$upstreamSrc = Join-Path $root 'upstream/melee-pc/src/melee'
$audioStatic = Get-Content -Raw (Join-Path $upstreamSrc 'lb/lbaudio_ax.static.h')
$selection = ''
foreach ($name in @('s32_arr_803BB5D0', 's32_arr_803BB8D4')) {
    $m = [regex]::Match($audioStatic, '(?s)static (?:int|s8) ' + $name + '\[[^;]*?\n\};')
    if (-not $m.Success) { throw "Missing original sound table: $name" }
    $selection += $m.Value + "`r`n"
}
$audioText = Get-Content -Raw (Join-Path $upstreamSrc 'lb/lbaudio_ax.c')
foreach ($signature in @('int lbAudioAx_800230C8(', 'int lbAudioAx_80023130(', 'int lbAudioAx_80023220(')) {
    $selection += (Get-SoundFunction $audioText $signature) + "`r`n"
}
$fighterText = Get-Content -Raw (Join-Path $upstreamSrc 'ft/ft_0877.c')
foreach ($signature in @('s32 ft_80087C70(', 's32 ft_80087D0C(')) {
    $selection += (Get-SoundFunction $fighterText $signature) + "`r`n"
}
$commonText = Get-Content -Raw (Join-Path $upstreamSrc 'ft/ftcommon.c')
$selection += Get-SoundFunction $commonText 'bool ftCommon_80080144('
Set-Content -LiteralPath (Join-Path $out 'fighter_sfx_original.h') -Encoding ASCII $selection
$renderText = Get-Content -Raw (Join-Path $root 'src/xdk/hsd_render_xdk.cpp')
$cache = (Get-SoundFunction $renderText 'struct TextureEntry {') + ";`r`n"
$cache += "const unsigned kMaxTextures = 4;`r`nTextureEntry s_textures[kMaxTextures];`r`nunsigned s_textureCount, s_textureAge;`r`n"
foreach ($signature in @('bool TextureMatches(', 'void CacheTexture(', 'void M360_HsdRenderClearTextures(')) {
    $cache += (Get-SoundFunction $renderText $signature) + "`r`n"
}
Set-Content -LiteralPath (Join-Path $out 'texture_cache_original.h') -Encoding ASCII $cache
$vertex = ''
foreach ($signature in @('struct HsdVertex {', 'struct MatrixSet {', 'struct RawVertex {', 'struct TexturePasses {')) {
    $vertex += (Get-SoundFunction $renderText $signature) + ";`r`n"
}
foreach ($signature in @('unsigned Read16(', 'unsigned Read32(', 'unsigned ComponentSize(',
    'float ReadComponent(', 'unsigned DirectSize(', 'unsigned Expand4(', 'void DecodeColor(',
    'bool ParseVertex(', 'void Transform(', 'TexturePasses TexturePassesFor(')) {
    $vertex += (Get-SoundFunction $renderText $signature) + "`r`n"
}
Set-Content -LiteralPath (Join-Path $out 'gx_vertex_original.h') -Encoding ASCII $vertex
$batch = @"
@echo off
call "$VcVars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /I"$root\upstream\melee-pc\src\sdk_include" /Fo"$out\test_gx_vertex.obj" /Fe"$out\test_gx_vertex.exe" "$root\tests\host\test_gx_vertex.cpp"
if errorlevel 1 exit /b 1
"$out\test_gx_vertex.exe"
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /Fo"$out\test_texture_cache.obj" /Fe"$out\test_texture_cache.exe" "$root\tests\host\test_texture_cache.cpp"
if errorlevel 1 exit /b 1
"$out\test_texture_cache.exe"
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /Fo"$out\test_fighter_sfx.obj" /Fe"$out\test_fighter_sfx.exe" "$root\tests\host\test_fighter_sfx.cpp"
if errorlevel 1 exit /b 1
"$out\test_fighter_sfx.exe"
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /TC /W4 /WX /Fo"$out\\" /Fe"$out\test_gx_texture.exe" "$root\src\xdk\hsd_texture_xdk.c" "$root\tests\host\test_gx_texture.c"
if errorlevel 1 exit /b 1
"$out\test_gx_texture.exe"
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /TC /D_CRT_SECURE_NO_WARNINGS /Fo"$out\\" /c "$root\src\common\gcm.c" "$root\src\common\hps.c" "$root\src\common\dsp_adpcm.c"
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /D_CRT_SECURE_NO_WARNINGS /I"$root\tests\host\xdk_audio_mock" /Fo"$out\test_audio_xdk.obj" /Fe"$out\test_audio_xdk.exe" "$root\tests\host\test_audio_xdk.cpp" "$out\gcm.obj" "$out\hps.obj" "$out\dsp_adpcm.obj"
if errorlevel 1 exit /b 1
"$out\test_audio_xdk.exe" "$Iso"
exit /b %errorlevel%
"@
$batchPath = Join-Path $out 'test.bat'
Set-Content -LiteralPath $batchPath -Encoding ASCII $batch
& cmd.exe /d /c $batchPath
if ($LASTEXITCODE -ne 0) { throw 'XDK audio host regression failed.' }
