[CmdletBinding()]
param($Compiler, [string] $Build)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$src=Join-Path $root 'upstream/melee-pc/src'
$out=Join-Path $Build 'menu'
New-Item -ItemType Directory -Force $out | Out-Null
& python (Join-Path $PSScriptRoot 'generate_menu_sources.py') --output $out | ForEach-Object { Write-Host $_ }
if($LASTEXITCODE -ne 0) { throw 'Original menu dependency extraction failed.' }
& python (Join-Path $PSScriptRoot 'generate_campaign_sources.py') (Join-Path $out 'campaign_original.h')
if($LASTEXITCODE -ne 0) { throw 'Original campaign extraction failed.' }
& python (Join-Path $PSScriptRoot 'generate_stage_music_sources.py') (Join-Path $out 'stage_music_original.h')
if($LASTEXITCODE -ne 0) { throw 'Original stage music extraction failed.' }
$units=@(
    @{File="$src/melee/mn/mncharsel.c"; Dir="$src/melee/mn"},
    @{File="$src/melee/mn/mnmainrule.c"; Dir="$src/melee/mn"},
    @{File="$src/melee/mn/mnruleplus.c"; Dir="$src/melee/mn"},
    @{File="$src/melee/mn/mnitemsw.c"; Dir="$src/melee/mn"},
    @{File="$src/melee/mn/mnstagesw.c"; Dir="$src/melee/mn"},
    @{File="$src/melee/mn/mnstagesel.c"; Dir="$src/melee/mn"},
    @{File="$src/sysdolphin/baselib/sislib.c"; Dir="$src/sysdolphin/baselib"},
    @{File="$src/sysdolphin/baselib/hsd_3A64.c"; Dir="$src/sysdolphin/baselib"},
    @{File="$src/sysdolphin/baselib/sislib_font.c"; Dir="$src/sysdolphin/baselib"},
    @{File="$out/hsd_3A76_native.c"; Dir="$src/sysdolphin/baselib"},
    @{File="$out/gm_1601_menu.c"; Dir="$src/melee/gm"},
    @{File="$out/gmmain_lib_menu.c"; Dir="$src/melee/gm"},
    @{File="$out/mnname_menu.c"; Dir="$src/melee/mn"},
    @{File="$out/audio_menu_select.c"; Dir="$src/melee/lb"},
    @{File="$root/src/xdk/sis_native_xdk.c"; Dir="$root/src/xdk"},
    @{File="$root/src/xdk/character_select_xdk.c"; Dir="$root/src/xdk"},
    @{File="$root/src/xdk/campaign_xdk.c"; Dir="$root/src/xdk"},
    @{File="$root/src/xdk/stage_music_xdk.c"; Dir="$root/src/xdk"},
    @{File="$out/discfont_sis.cpp"; Dir="$src/sysdolphin/baselib"; Cpp=$true}
)
foreach($unit in $units) {
    $obj=Join-Path $out ([IO.Path]::GetFileNameWithoutExtension($unit.File)+'.obj')
    $language=if($unit.Cpp){'/TP'}else{'/TC'}
    & $Compiler @('/nologo','/c',$language,'/O2','/MT','/GS-','/W3','/D_XBOX','/DXBOX','/DNDEBUG',
        "/I$root/build-x360/gameplay-probe/include","/I$($unit.Dir)","/I$out","/I$root/src/xdk","/I$src",
        "/I$src/sdk_include","/I$env:XEDK/include/xbox","/I$env:XEDK/include/xbox/sys",
        "/FI$root/src/xdk/hsd_scene_compat.h","/Fo$obj",$unit.File) | ForEach-Object { Write-Host $_ }
    if($LASTEXITCODE -ne 0){throw "Menu compilation failed: $($unit.File)"}
    Write-Output $obj
}
