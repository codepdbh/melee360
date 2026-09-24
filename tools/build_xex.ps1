[CmdletBinding()]
param([switch] $BootToMatch, [switch] $InputScript)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$xedk = $env:XEDK
if (-not $xedk -or -not (Test-Path -LiteralPath $xedk)) {
    throw 'XEDK is not set to a complete Xbox 360 SDK installation.'
}

$compiler = Join-Path $xedk 'bin\win32\cl.exe'
$linker = Join-Path $xedk 'bin\win32\link.exe'
$imagexex = Join-Path $xedk 'bin\win32\imagexex.exe'
$shaderCompiler = Join-Path $xedk 'bin\win32\fxc.exe'
$includeXbox = Join-Path $xedk 'include\xbox'
$includeSys = Join-Path $xedk 'include\xbox\sys'
$libXbox = Join-Path $xedk 'lib\xbox'
$source = Join-Path $root 'src\xdk\main.cpp'
$compatSource = Join-Path $root 'src\xdk\melee_time_compat.cpp'
$compatHeader = Join-Path $root 'src\xdk\melee_xdk_compat.h'
$lbtimeSource = Join-Path $root 'upstream\melee-pc\src\melee\lb\lbtime.c'
$padSource = Join-Path $root 'src\xdk\melee_pad_xdk.cpp'
$padHeader = Join-Path $root 'src\xdk\melee_pad_xdk.h'
$controllerCompat = Join-Path $root 'src\xdk\controller_xdk_compat.h'
$controllerSource = Join-Path $root 'upstream\melee-pc\src\sysdolphin\baselib\controller.c'
$meleeSdkInclude = Join-Path $root 'upstream\melee-pc\src\sdk_include'
$lbmathCompat = Join-Path $root 'src\xdk\lbmath_xdk_compat.h'
$lbmathSource = Join-Path $root 'upstream\melee-pc\src\melee\lb\lb_00CE.c'
$lbmathWrapper = Join-Path $root 'src\xdk\lbmath_xdk.cpp'
$memoryCompat = Join-Path $root 'src\xdk\memory_xdk_compat.h'
$memorySource = Join-Path $root 'upstream\melee-pc\src\sysdolphin\baselib\memory.c'
$memoryWrapper = Join-Path $root 'src\xdk\memory_xdk.cpp'
$hsdClassCompat = Join-Path $root 'src\xdk\hsd_class_xdk_compat.h'
$hsdClassWrapper = Join-Path $root 'src\xdk\hsd_class_xdk.cpp'
$baselibDir = Join-Path $root 'upstream\melee-pc\src\sysdolphin\baselib'
$hashSource = Join-Path $baselibDir 'hash.c'
$debugSource = Join-Path $baselibDir 'debug.c'
$classSource = Join-Path $baselibDir 'class.c'
$objectSource = Join-Path $baselibDir 'object.c'
$objallocSource = Join-Path $baselibDir 'objalloc.c'
$idSource = Join-Path $baselibDir 'id.c'
$gobjCompat = Join-Path $root 'src\xdk\gobj_xdk_compat.h'
$gobjWrapper = Join-Path $root 'src\xdk\gobj_xdk.cpp'
$listSource = Join-Path $baselibDir 'list.c'
$gobjObjectSource = Join-Path $baselibDir 'gobjobject.c'
$gobjUserDataSource = Join-Path $baselibDir 'gobjuserdata.c'
$gobjProcSource = Join-Path $baselibDir 'gobjproc.c'
$gobjPLinkSource = Join-Path $baselibDir 'gobjplink.c'
$gobjGXLinkSource = Join-Path $baselibDir 'gobjgxlink.c'
$gobjInitSource = Join-Path $baselibDir 'gobjinit.c'
$gobjSource = Join-Path $baselibDir 'gobj.c'
$hsdMathCompat = Join-Path $root 'src\xdk\hsdmath_xdk_compat.h'
$hsdMathWrapper = Join-Path $root 'src\xdk\hsdmath_xdk.cpp'
$mtxSource = Join-Path $baselibDir 'mtx.c'
$quatlibSource = Join-Path $baselibDir 'quatlib.c'
$splineSource = Join-Path $baselibDir 'spline.c'
$splineWrapper = Join-Path $root 'src\xdk\spline_xdk.cpp'
$fobjSource = Join-Path $baselibDir 'fobj.c'
$randomSource = Join-Path $baselibDir 'random.c'
$hsdAnimCompat = Join-Path $root 'src\xdk\hsdanim_xdk_compat.h'
$hsdAnimWrapper = Join-Path $root 'src\xdk\hsdanim_xdk.cpp'
$aobjSource = Join-Path $baselibDir 'aobj.c'
$dobjSource = Join-Path $baselibDir 'dobj.c'
$robjSource = Join-Path $baselibDir 'robj.c'
$utilSource = Join-Path $baselibDir 'util.c'
$bytecodeSource = Join-Path $baselibDir 'bytecode.c'
$hsdJObjCompat = Join-Path $root 'src\xdk\hsdjobj_xdk_compat.h'
$hsdJObjWrapper = Join-Path $root 'src\xdk\hsdjobj_xdk.cpp'
$jobjSource = Join-Path $baselibDir 'jobj.c'
$wobjSource = Join-Path $baselibDir 'wobj.c'
$hsdSynthCompat = Join-Path $root 'src\xdk\hsdsynth_xdk_compat.h'
$hsdSynthWrapper = Join-Path $root 'src\xdk\hsdsynth_xdk.cpp'
$synthSource = Join-Path $baselibDir 'synth.c'
$devcomSource = Join-Path $baselibDir 'devcom.c'
$spriteSource = Join-Path $root 'src\xdk\sprite_renderer.cpp'
$spriteHeader = Join-Path $root 'src\xdk\sprite_renderer.h'
$bootSource = Join-Path $root 'src\xdk\melee_boot_xdk.cpp'
$bootHeader = Join-Path $root 'src\xdk\melee_boot_xdk.h'
$archiveSource = Join-Path $root 'src\xdk\melee_archive_xdk.cpp'
$archiveHeader = Join-Path $root 'src\xdk\melee_archive_xdk.h'
$titleSceneSource = Join-Path $root 'src\xdk\melee_title_scene_xdk.cpp'
$titleSceneHeader = Join-Path $root 'src\xdk\melee_title_scene_xdk.h'
$hsdRenderSource = Join-Path $root 'src\xdk\hsd_render_xdk.cpp'
$hsdTextureSource = Join-Path $root 'src\xdk\hsd_texture_xdk.c'
$menuSceneSource = Join-Path $root 'src\xdk\menu_scene_xdk.c'
$sceneCompat = Join-Path $root 'src\xdk\hsd_scene_compat.h'
$gcmSource = Join-Path $root 'src\common\gcm.c'
$vertexShaderSource = Join-Path $root 'src\xdk\shaders\sprite_vs.hlsl'
$pixelShaderSource = Join-Path $root 'src\xdk\shaders\sprite_ps.hlsl'
$atlasGenerator = Join-Path $root 'tools\generate_sprite_atlas.ps1'
$build = Join-Path $root 'build-x360\xdk'
$dist = Join-Path $root 'dist'
$object = Join-Path $build 'main.obj'
$compatObject = Join-Path $build 'melee_time_compat.obj'
$lbtimeObject = Join-Path $build 'lbtime.obj'
$padObject = Join-Path $build 'melee_pad_xdk.obj'
$controllerObject = Join-Path $build 'controller.obj'
$lbmathObject = Join-Path $build 'lb_00CE.obj'
$spriteObject = Join-Path $build 'sprite_renderer.obj'
$bootObject = Join-Path $build 'melee_boot_xdk.obj'
$archiveObject = Join-Path $build 'melee_archive_xdk.obj'
$titleSceneObject = Join-Path $build 'melee_title_scene_xdk.obj'
$hsdRenderObject = Join-Path $build 'hsd_render_xdk.obj'
$hsdTextureObject = Join-Path $build 'hsd_texture_xdk.obj'
$gcmObject = Join-Path $build 'gcm.obj'
$memoryObject = Join-Path $build 'memory.obj'
$memoryWrapperObject = Join-Path $build 'memory_xdk.obj'
$hsdClassWrapperObject = Join-Path $build 'hsd_class_xdk.obj'
$hashObject = Join-Path $build 'hash.obj'
$debugObject = Join-Path $build 'debug.obj'
$classObject = Join-Path $build 'class.obj'
$objectObject = Join-Path $build 'object.obj'
$objallocObject = Join-Path $build 'objalloc.obj'
$idObject = Join-Path $build 'id.obj'
$gobjWrapperObject = Join-Path $build 'gobj_xdk.obj'
$listObject = Join-Path $build 'list.obj'
$gobjObjectObject = Join-Path $build 'gobjobject.obj'
$gobjUserDataObject = Join-Path $build 'gobjuserdata.obj'
$gobjProcObject = Join-Path $build 'gobjproc.obj'
$gobjPLinkObject = Join-Path $build 'gobjplink.obj'
$gobjGXLinkObject = Join-Path $build 'gobjgxlink.obj'
$gobjInitObject = Join-Path $build 'gobjinit.obj'
$gobjObject = Join-Path $build 'gobj.obj'
$hsdMathWrapperObject = Join-Path $build 'hsdmath_xdk.obj'
$mtxObject = Join-Path $build 'mtx.obj'
$quatlibObject = Join-Path $build 'quatlib.obj'
$splineObject = Join-Path $build 'spline.obj'
$fobjObject = Join-Path $build 'fobj.obj'
$randomObject = Join-Path $build 'random.obj'
$hsdAnimWrapperObject = Join-Path $build 'hsdanim_xdk.obj'
$aobjObject = Join-Path $build 'aobj.obj'
$dobjObject = Join-Path $build 'dobj.obj'
$robjObject = Join-Path $build 'robj.obj'
$utilObject = Join-Path $build 'util.obj'
$bytecodeObject = Join-Path $build 'bytecode.obj'
$hsdJObjWrapperObject = Join-Path $build 'hsdjobj_xdk.obj'
$jobjObject = Join-Path $build 'jobj.obj'
$wobjObject = Join-Path $build 'wobj.obj'
$hsdSynthWrapperObject = Join-Path $build 'hsdsynth_xdk.obj'
$synthObject = Join-Path $build 'synth.obj'
$devcomObject = Join-Path $build 'devcom.obj'
$vertexShaderHeader = Join-Path $build 'sprite_vs.h'
$pixelShaderHeader = Join-Path $build 'sprite_ps.h'
$pe = Join-Path $build 'melee360.exe'
$pdb = Join-Path $build 'melee360.pdb'
$xex = Join-Path $dist 'default.xex'

foreach ($required in @($compiler, $linker, $imagexex, $shaderCompiler,
                         (Join-Path $includeXbox 'xtl.h'),
                         (Join-Path $libXbox 'xboxkrnl.lib'),
                         $source, $compatSource, $compatHeader,
                         $lbtimeSource, $padSource, $padHeader,
                         $controllerCompat, $controllerSource,
                         $meleeSdkInclude, $lbmathCompat, $lbmathSource,
                         $lbmathWrapper, $spriteSource, $spriteHeader,
                         $vertexShaderSource, $pixelShaderSource,
                         $atlasGenerator, $bootSource, $bootHeader,
                         $archiveSource, $archiveHeader,
                         $titleSceneSource, $titleSceneHeader,
                         $hsdRenderSource, $hsdTextureSource, $menuSceneSource,
                         $sceneCompat,
                         $gcmSource, $memoryCompat, $memorySource,
                         $memoryWrapper, $hsdClassCompat, $hsdClassWrapper,
                         $hashSource, $debugSource, $classSource,
                         $objectSource, $objallocSource, $idSource,
                         $gobjCompat, $gobjWrapper, $listSource,
                         $gobjObjectSource, $gobjUserDataSource,
                         $gobjProcSource, $gobjPLinkSource,
                         $gobjGXLinkSource, $gobjInitSource, $gobjSource,
                         $hsdMathCompat, $hsdMathWrapper, $mtxSource,
                         $quatlibSource, $splineSource, $splineWrapper, $fobjSource,
                         $randomSource, $hsdAnimCompat, $hsdAnimWrapper,
                         $aobjSource, $dobjSource, $robjSource,
                         $utilSource, $bytecodeSource, $hsdJObjCompat,
                         $hsdJObjWrapper, $jobjSource, $wobjSource,
                         $hsdSynthCompat, $hsdSynthWrapper, $synthSource,
                         $devcomSource)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required XDK component is missing: $required"
    }
}

New-Item -ItemType Directory -Path $build, $dist -Force | Out-Null

& $atlasGenerator -OutputPath (Join-Path $dist 'assets\sprite_atlas.png')

$localIso = Join-Path $dist 'melee.iso'
$isoCandidates = @(Get-ChildItem -LiteralPath (Join-Path $root 'iso') `
    -Filter '*.iso' -File -ErrorAction SilentlyContinue)
if (-not (Test-Path -LiteralPath $localIso) -and $isoCandidates.Count -eq 1) {
    New-Item -ItemType HardLink -Path $localIso `
        -Target $isoCandidates[0].FullName | Out-Null
    Write-Host "[M360][ISO] linked local legal image as $localIso"
}

Write-Host '[M360][XEX] compiling D3D9 sprite shaders'
& $shaderCompiler '/nologo' '/Tvs_3_0' '/Emain' "/Fh$vertexShaderHeader" `
    '/Vng_melee360SpriteVS' $vertexShaderSource
if ($LASTEXITCODE -ne 0) { throw 'Sprite vertex shader compilation failed.' }
& $shaderCompiler '/nologo' '/Tps_3_0' '/Emain' "/Fh$pixelShaderHeader" `
    '/Vng_melee360SpritePS' $pixelShaderSource
if ($LASTEXITCODE -ne 0) { throw 'Sprite pixel shader compilation failed.' }
& $shaderCompiler '/nologo' '/Tvs_3_0' '/Emain' "/Fh$(Join-Path $build 'hsd_vs.h')" `
    '/Vng_melee360HsdVS' (Join-Path $root 'src\xdk\shaders\hsd_vs.hlsl')
if ($LASTEXITCODE -ne 0) { throw 'HSD vertex shader compilation failed.' }
& $shaderCompiler '/nologo' '/Tps_3_0' '/Emain' "/Fh$(Join-Path $build 'hsd_ps.h')" `
    '/Vng_melee360HsdPS' (Join-Path $root 'src\xdk\shaders\hsd_ps.hlsl')
if ($LASTEXITCODE -ne 0) { throw 'HSD pixel shader compilation failed.' }
$moviePixelShaderHeader = Join-Path $build 'movie_ps.h'
& $shaderCompiler '/nologo' '/Tps_3_0' '/Emain' "/Fh$moviePixelShaderHeader" `
    '/Vng_melee360MoviePS' (Join-Path $root 'src\xdk\shaders\movie_ps.hlsl')
if ($LASTEXITCODE -ne 0) { throw 'Movie YUV pixel shader compilation failed.' }

Write-Host '[M360][XEX] compiling D3D9 sprite renderer'
$spriteArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys", "/I$build",
    "/Fo$spriteObject", $spriteSource
)
& $compiler $spriteArgs
if ($LASTEXITCODE -ne 0) { throw 'Sprite renderer compilation failed.' }

Write-Host '[M360][XEX] compiling native Melee ISO boot path'
$bootArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys", "/I$(Join-Path $root 'src\common')",
    "/Fo$bootObject", $bootSource
)
& $compiler $bootArgs
if ($LASTEXITCODE -ne 0) { throw 'Native Melee boot compilation failed.' }

Write-Host '[M360][XEX] compiling original melee-pc HAL archive parser'
$archiveArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys", "/I$meleeSdkInclude",
    "/I$(Join-Path $root 'src\xdk')",
    "/Fo$archiveObject", $archiveSource
)
& $compiler $archiveArgs
if ($LASTEXITCODE -ne 0) { throw 'Original HAL archive parser compilation failed.' }

Write-Host '[M360][XEX] compiling real title scene loader'
$titleSceneArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys", "/I$(Join-Path $root 'src\xdk')",
    "/I$(Join-Path $root 'upstream\melee-pc\src')", "/I$meleeSdkInclude",
    "/Fo$titleSceneObject", $titleSceneSource
)
& $compiler $titleSceneArgs
if ($LASTEXITCODE -ne 0) { throw 'Real title scene loader compilation failed.' }

Write-Host '[M360][XEX] compiling native HSD scene renderer'
& $compiler @('/nologo', '/c', '/O2', '/MT', '/GS-', '/W4', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', "/Fo$hsdTextureObject", $hsdTextureSource)
if ($LASTEXITCODE -ne 0) { throw 'GX texture decoder compilation failed.' }
& $compiler @('/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys", "/I$(Join-Path $root 'src\xdk')", "/I$build",
    "/I$(Join-Path $root 'upstream\melee-pc\src')", "/I$meleeSdkInclude",
    "/Fo$hsdRenderObject", $hsdRenderSource)
if ($LASTEXITCODE -ne 0) { throw 'HSD scene renderer compilation failed.' }

$gcmArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/GS-', '/W4', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys", "/Fo$gcmObject", $gcmSource
)
& $compiler $gcmArgs
if ($LASTEXITCODE -ne 0) { throw 'GameCube FST reader compilation failed.' }

Write-Host '[M360][XEX] compiling HPS/DSP-ADPCM decoder and XAudio2 stream'
$audioObjects = @()
foreach ($audioSource in @('src\common\dsp_adpcm.c', 'src\common\hps.c')) {
    $audioObject = Join-Path $build ([IO.Path]::GetFileNameWithoutExtension($audioSource) + '.obj')
    & $compiler @('/nologo', '/c', '/O2', '/MT', '/GS-', '/W4', '/TC',
        '/D_XBOX', '/DXBOX', '/DNDEBUG', "/I$includeXbox", "/I$includeSys",
        "/Fo$audioObject", (Join-Path $root $audioSource))
    if ($LASTEXITCODE -ne 0) { throw "$audioSource compilation failed." }
    $audioObjects += $audioObject
}
$audioObject = Join-Path $build 'melee_audio_xdk.obj'
& $compiler @('/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', "/I$includeXbox", "/I$includeSys",
    "/Fo$audioObject", (Join-Path $root 'src\xdk\melee_audio_xdk.cpp'))
if ($LASTEXITCODE -ne 0) { throw 'XAudio2 stream compilation failed.' }
$audioObjects += $audioObject

Write-Host '[M360][XEX] compiling MTH/THP-JPEG movie player and boot flow'
$movieObjects = @()
foreach ($movieSource in @('src\common\jpeg_decode.c', 'src\common\mth.c')) {
    $movieObject = Join-Path $build ([IO.Path]::GetFileNameWithoutExtension($movieSource) + '.obj')
    & $compiler @('/nologo', '/c', '/O2', '/MT', '/GS-', '/W4', '/TC',
        '/D_XBOX', '/DXBOX', '/DNDEBUG', "/I$includeXbox", "/I$includeSys",
        "/Fo$movieObject", (Join-Path $root $movieSource))
    if ($LASTEXITCODE -ne 0) { throw "$movieSource compilation failed." }
    $movieObjects += $movieObject
}
$flowDefine = if ($BootToMatch) { '/DM360_BOOT_TO_MATCH' } else { '/DM360_BOOT_NORMAL' }
foreach ($movieSource in @('src\xdk\melee_movie_xdk.cpp', 'src\xdk\melee_flow_xdk.cpp')) {
    $movieObject = Join-Path $build ([IO.Path]::GetFileNameWithoutExtension($movieSource) + '.obj')
    & $compiler @('/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4', $flowDefine,
        '/D_XBOX', '/DXBOX', '/DNDEBUG', "/I$includeXbox", "/I$includeSys",
        "/I$build", "/Fo$movieObject", (Join-Path $root $movieSource))
    if ($LASTEXITCODE -ne 0) { throw "$movieSource compilation failed." }
    $movieObjects += $movieObject
}

Write-Host '[M360][XEX] compiling PowerPC source'
$compileArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys", "/I$(Join-Path $root 'src\xdk')",
    "/I$baselibDir", "/I$(Join-Path $root 'upstream\melee-pc\src')",
    "/I$meleeSdkInclude", "/Fo$object", $source
)
& $compiler $compileArgs
if ($LASTEXITCODE -ne 0) { throw 'XDK compilation failed.' }

Write-Host '[M360][XEX] compiling XDK time compatibility layer'
$compatArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys", "/Fo$compatObject", $compatSource
)
& $compiler $compatArgs
if ($LASTEXITCODE -ne 0) { throw 'XDK time compatibility compilation failed.' }

Write-Host '[M360][XEX] compiling original melee-pc lbtime.c'
$lbtimeArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/GS-', '/W4', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', "/FI$compatHeader",
    "/Fo$lbtimeObject", $lbtimeSource
)
& $compiler $lbtimeArgs
if ($LASTEXITCODE -ne 0) { throw 'Original lbtime.c compilation failed.' }

Write-Host '[M360][XEX] compiling XInput to Dolphin PAD bridge'
$padArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', $(if ($InputScript) { '/DM360_INPUT_SCRIPT' } else { '/DM360_LIVE_INPUT' }),
    "/I$includeXbox", "/I$includeSys", "/Fo$padObject", $padSource
)
& $compiler $padArgs
if ($LASTEXITCODE -ne 0) { throw 'XDK PAD bridge compilation failed.' }

Write-Host '[M360][XEX] compiling original melee-pc controller.c'
$controllerArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/GS-', '/W3', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', "/FI$controllerCompat",
    "/I$includeXbox", "/I$includeSys", "/I$meleeSdkInclude",
    "/Fo$controllerObject", $controllerSource
)
& $compiler $controllerArgs
if ($LASTEXITCODE -ne 0) { throw 'Original controller.c compilation failed.' }

Write-Host '[M360][XEX] compiling original melee-pc lb_00CE.c'
$lbmathArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W3',
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys", "/I$(Join-Path $root 'upstream\melee-pc\src')",
    "/Fo$lbmathObject", $lbmathWrapper
)
& $compiler $lbmathArgs
if ($LASTEXITCODE -ne 0) { throw 'Original lb_00CE.c compilation failed.' }

Write-Host '[M360][XEX] compiling XDK HSD heap allocator bridge'
$memoryWrapperArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys",
    "/Fo$memoryWrapperObject", $memoryWrapper
)
& $compiler $memoryWrapperArgs
if ($LASTEXITCODE -ne 0) { throw 'XDK HSD heap allocator bridge compilation failed.' }

Write-Host '[M360][XEX] compiling original melee-pc memory.c'
$memoryArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/GS-', '/W4', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', "/FI$memoryCompat",
    "/I$(Join-Path $root 'upstream\melee-pc\src')", "/I$meleeSdkInclude",
    "/Fo$memoryObject", $memorySource
)
& $compiler $memoryArgs
if ($LASTEXITCODE -ne 0) { throw 'Original memory.c compilation failed.' }

Write-Host '[M360][XEX] compiling XDK HSD class/debug bridge'
$hsdClassWrapperArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys",
    "/Fo$hsdClassWrapperObject", $hsdClassWrapper
)
& $compiler $hsdClassWrapperArgs
if ($LASTEXITCODE -ne 0) { throw 'XDK HSD class/debug bridge compilation failed.' }

Write-Host '[M360][XEX] compiling original melee-pc hash/debug/class/object/objalloc/id.c'
$hsdCommonInc = @("/I$(Join-Path $root 'upstream\melee-pc\src')", "/I$meleeSdkInclude",
    "/I$includeXbox", "/I$includeSys")

$hashArgs = @('/nologo', '/c', '/O2', '/MT', '/GS-', '/W3', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', "/FI$hsdClassCompat") + $hsdCommonInc +
    @("/Fo$hashObject", $hashSource)
& $compiler $hashArgs
if ($LASTEXITCODE -ne 0) { throw 'Original hash.c compilation failed.' }

$debugArgs = @('/nologo', '/c', '/O2', '/MT', '/GS-', '/W3', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', '/DTARGET_PC', "/FI$hsdClassCompat") +
    $hsdCommonInc + @("/Fo$debugObject", $debugSource)
& $compiler $debugArgs
if ($LASTEXITCODE -ne 0) { throw 'Original debug.c compilation failed.' }

$classArgs = @('/nologo', '/c', '/O2', '/MT', '/GS-', '/W3', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', "/FI$hsdClassCompat") + $hsdCommonInc +
    @("/Fo$classObject", $classSource)
& $compiler $classArgs
if ($LASTEXITCODE -ne 0) { throw 'Original class.c compilation failed.' }

$objectArgs = @('/nologo', '/c', '/O2', '/MT', '/GS-', '/W3', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', "/FI$hsdClassCompat") + $hsdCommonInc +
    @("/Fo$objectObject", $objectSource)
& $compiler $objectArgs
if ($LASTEXITCODE -ne 0) { throw 'Original object.c compilation failed.' }

$objallocArgs = @('/nologo', '/c', '/O2', '/MT', '/GS-', '/W3', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', "/FI$hsdClassCompat") + $hsdCommonInc +
    @("/Fo$objallocObject", $objallocSource)
& $compiler $objallocArgs
if ($LASTEXITCODE -ne 0) { throw 'Original objalloc.c compilation failed.' }

$idArgs = @('/nologo', '/c', '/O2', '/MT', '/GS-', '/W3', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', "/FI$hsdClassCompat") + $hsdCommonInc +
    @("/Fo$idObject", $idSource)
& $compiler $idArgs
if ($LASTEXITCODE -ne 0) { throw 'Original id.c compilation failed.' }

Write-Host '[M360][XEX] compiling XDK gobj bridge'
$gobjWrapperArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/DM360_NATIVE_RENDER', '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys",
    "/I$(Join-Path $root 'upstream\melee-pc\src')",
    "/Fo$gobjWrapperObject", $gobjWrapper
)
& $compiler $gobjWrapperArgs
if ($LASTEXITCODE -ne 0) { throw 'XDK gobj bridge compilation failed.' }

Write-Host '[M360][XEX] compiling original melee-pc list/gobj*.c'
$gobjArgs = @('/nologo', '/c', '/O2', '/MT', '/GS-', '/W3', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', "/FI$gobjCompat") + $hsdCommonInc

& $compiler ($gobjArgs + @("/Fo$listObject", $listSource))
if ($LASTEXITCODE -ne 0) { throw 'Original list.c compilation failed.' }

& $compiler ($gobjArgs + @("/Fo$gobjObjectObject", $gobjObjectSource))
if ($LASTEXITCODE -ne 0) { throw 'Original gobjobject.c compilation failed.' }

& $compiler ($gobjArgs + @("/Fo$gobjUserDataObject", $gobjUserDataSource))
if ($LASTEXITCODE -ne 0) { throw 'Original gobjuserdata.c compilation failed.' }

& $compiler ($gobjArgs + @("/Fo$gobjProcObject", $gobjProcSource))
if ($LASTEXITCODE -ne 0) { throw 'Original gobjproc.c compilation failed.' }

& $compiler ($gobjArgs + @("/Fo$gobjPLinkObject", $gobjPLinkSource))
if ($LASTEXITCODE -ne 0) { throw 'Original gobjplink.c compilation failed.' }

& $compiler ($gobjArgs + @("/Fo$gobjGXLinkObject", $gobjGXLinkSource))
if ($LASTEXITCODE -ne 0) { throw 'Original gobjgxlink.c compilation failed.' }

& $compiler ($gobjArgs + @("/Fo$gobjInitObject", $gobjInitSource))
if ($LASTEXITCODE -ne 0) { throw 'Original gobjinit.c compilation failed.' }

& $compiler ($gobjArgs + @("/Fo$gobjObject", $gobjSource))
if ($LASTEXITCODE -ne 0) { throw 'Original gobj.c compilation failed.' }

Write-Host '[M360][XEX] compiling XDK HSD math bridge'
$hsdMathWrapperArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys", "/I$(Join-Path $root 'src\xdk')",
    "/I$(Join-Path $root 'upstream\melee-pc\src')", "/I$meleeSdkInclude",
    "/Fo$hsdMathWrapperObject", $hsdMathWrapper
)
& $compiler $hsdMathWrapperArgs
if ($LASTEXITCODE -ne 0) { throw 'XDK HSD math bridge compilation failed.' }

Write-Host '[M360][XEX] compiling original melee-pc mtx/quatlib/spline/fobj/random.c'
$hsdMathArgs = @('/nologo', '/c', '/O2', '/MT', '/GS-', '/W3', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', "/FI$hsdMathCompat") + $hsdCommonInc

& $compiler ($hsdMathArgs + @("/Fo$mtxObject", $mtxSource))
if ($LASTEXITCODE -ne 0) { throw 'Original mtx.c compilation failed.' }

& $compiler ($hsdMathArgs + @("/Fo$quatlibObject", $quatlibSource))
if ($LASTEXITCODE -ne 0) { throw 'Original quatlib.c compilation failed.' }

$splineArgs = @('/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W3',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', "/I$(Join-Path $root 'src\xdk')") +
    $hsdCommonInc + @("/Fo$splineObject", $splineWrapper)
& $compiler $splineArgs
if ($LASTEXITCODE -ne 0) { throw 'Original spline.c compilation failed.' }

& $compiler ($hsdMathArgs + @("/Fo$fobjObject", $fobjSource))
if ($LASTEXITCODE -ne 0) { throw 'Original fobj.c compilation failed.' }

& $compiler ($hsdMathArgs + @("/Fo$randomObject", $randomSource))
if ($LASTEXITCODE -ne 0) { throw 'Original random.c compilation failed.' }

Write-Host '[M360][XEX] compiling XDK HSD anim bridge'
$hsdAnimWrapperArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys", "/I$(Join-Path $root 'src\xdk')",
    "/I$(Join-Path $root 'upstream\melee-pc\src')", "/I$meleeSdkInclude",
    "/Fo$hsdAnimWrapperObject", $hsdAnimWrapper
)
& $compiler $hsdAnimWrapperArgs
if ($LASTEXITCODE -ne 0) { throw 'XDK HSD anim bridge compilation failed.' }

Write-Host '[M360][XEX] compiling original melee-pc aobj/dobj/robj/util/bytecode.c'
$hsdAnimArgs = @('/nologo', '/c', '/O2', '/MT', '/GS-', '/W3', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', "/FI$hsdAnimCompat") + $hsdCommonInc

& $compiler ($hsdAnimArgs + @("/Fo$aobjObject", $aobjSource))
if ($LASTEXITCODE -ne 0) { throw 'Original aobj.c compilation failed.' }

& $compiler ($hsdAnimArgs + @("/Fo$dobjObject", $dobjSource))
if ($LASTEXITCODE -ne 0) { throw 'Original dobj.c compilation failed.' }

& $compiler ($hsdAnimArgs + @("/Fo$robjObject", $robjSource))
if ($LASTEXITCODE -ne 0) { throw 'Original robj.c compilation failed.' }

& $compiler ($hsdAnimArgs + @("/Fo$utilObject", $utilSource))
if ($LASTEXITCODE -ne 0) { throw 'Original util.c compilation failed.' }

& $compiler ($hsdAnimArgs + @("/Fo$bytecodeObject", $bytecodeSource))
if ($LASTEXITCODE -ne 0) { throw 'Original bytecode.c compilation failed.' }

Write-Host '[M360][XEX] compiling XDK HSD jobj bridge'
$hsdJObjWrapperArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/DM360_NATIVE_RENDER', '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys", "/I$(Join-Path $root 'src\xdk')",
    "/I$(Join-Path $root 'upstream\melee-pc\src')", "/I$meleeSdkInclude",
    "/Fo$hsdJObjWrapperObject", $hsdJObjWrapper
)
& $compiler $hsdJObjWrapperArgs
if ($LASTEXITCODE -ne 0) { throw 'XDK HSD jobj bridge compilation failed.' }

Write-Host '[M360][XEX] compiling original melee-pc jobj/wobj.c'
$hsdJObjArgs = @('/nologo', '/c', '/O2', '/MT', '/GS-', '/W3', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', "/FI$hsdJObjCompat") + $hsdCommonInc

& $compiler ($hsdJObjArgs + @("/Fo$jobjObject", $jobjSource))
if ($LASTEXITCODE -ne 0) { throw 'Original jobj.c compilation failed.' }

& $compiler ($hsdJObjArgs + @("/Fo$wobjObject", $wobjSource))
if ($LASTEXITCODE -ne 0) { throw 'Original wobj.c compilation failed.' }

Write-Host '[M360][XEX] compiling XDK HSD synth/devcom bridge'
$hsdSynthWrapperArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys", "/I$(Join-Path $root 'src\xdk')",
    "/I$(Join-Path $root 'upstream\melee-pc\src')", "/I$meleeSdkInclude",
    "/Fo$hsdSynthWrapperObject", $hsdSynthWrapper
)
& $compiler $hsdSynthWrapperArgs
if ($LASTEXITCODE -ne 0) { throw 'XDK HSD synth/devcom bridge compilation failed.' }

Write-Host '[M360][XEX] compiling original melee-pc synth.c/devcom.c'
$hsdSynthArgs = @('/nologo', '/c', '/O2', '/MT', '/GS-', '/W3', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG', "/FI$hsdSynthCompat") + $hsdCommonInc

& $compiler ($hsdSynthArgs + @("/Fo$synthObject", $synthSource))
if ($LASTEXITCODE -ne 0) { throw 'Original synth.c compilation failed.' }

& $compiler ($hsdSynthArgs + @("/Fo$devcomObject", $devcomSource))
if ($LASTEXITCODE -ne 0) { throw 'Original devcom.c compilation failed.' }

Write-Host '[M360][XEX] linking Xbox 360 PowerPC PE'
& (Join-Path $PSScriptRoot 'probe_gameplay_xdk.ps1')
$gameplayLayoutObject = Join-Path $root 'build-x360/gameplay-probe/gameplay_layout_probe.obj'
$menuInputObject = Join-Path $build 'menu_input_original.obj'
$menuInputTestObject = Join-Path $build 'menu_input_test.obj'
$menuInputSource = Get-Content -Raw (Join-Path $root 'upstream/melee-pc/src/melee/gm/gm_1A36.c')
# Keep the input implementation verbatim; the last function initializes every
# game mode and is not part of the controller mapper.
$cut = $menuInputSource.IndexOf('void gm_801A3EF4(void)')
if ($cut -lt 0) { throw 'Cannot locate mode initialization boundary in gm_1A36.c.' }
$menuInputSlice = Join-Path $build 'menu_input_original.c'
Set-Content -Encoding ASCII $menuInputSlice $menuInputSource.Substring(0, $cut)
$menuInputArgs = @('/nologo','/c','/TC','/O2','/MT','/D_XBOX','/DXBOX',
    "/I$(Join-Path $root 'upstream/melee-pc/src/melee/gm')",
    "/FI$(Join-Path $root 'src/xdk/gameplay_probe_compat.h')") + $hsdCommonInc
& $compiler ($menuInputArgs + @("/Fo$menuInputObject", $menuInputSlice))
if ($LASTEXITCODE -ne 0) { throw 'Original menu input compilation failed.' }
& $compiler ($menuInputArgs + @("/Fo$menuInputTestObject", (Join-Path $root 'src/xdk/menu_input_xdk.c')))
if ($LASTEXITCODE -ne 0) { throw 'Menu input self-test compilation failed.' }
function Get-CFunction([string]$Text, [string]$Signature) {
    $start = $Text.IndexOf($Signature)
    if ($start -lt 0) { throw "Missing original function: $Signature" }
    $cursor = $Text.IndexOf('{', $start)
    $depth = 1
    ++$cursor
    while ($depth -gt 0 -and $cursor -lt $Text.Length) {
        if ($Text[$cursor] -eq '{') { ++$depth }
        if ($Text[$cursor] -eq '}') { --$depth }
        ++$cursor
    }
    if ($depth -ne 0) { throw "Unbalanced source: $Signature" }
    return $Text.Substring($start, $cursor - $start)
}
function Get-CPrologue([string]$Text, [string]$FirstSignature) {
    $end = $Text.IndexOf($FirstSignature)
    if ($end -lt 0) { throw "Missing original function: $FirstSignature" }
    return $Text.Substring(0, $end)
}
$meleeSrc = Join-Path $root 'upstream/melee-pc/src'
$displaySource = Get-Content -Raw (Join-Path $baselibDir 'displayfunc.c')
$matrixStart = $displaySource.IndexOf('Vec3 zOne = ')
$displaySlice = (Get-CPrologue $displaySource 'void HSD_ZListInitAllocData') +
    $displaySource.Substring($matrixStart, $displaySource.IndexOf('void HSD_JObjDispSub') - $matrixStart) +
    (Get-CFunction $displaySource 'void HSD_JObjSetSPtclCallback(') + "`r`n"
Set-Content -Encoding ASCII (Join-Path $build 'displayfunc_matrix_slice.c') $displaySlice
$spDisplaySource = Get-Content -Raw (Join-Path $meleeSrc 'melee/lb/lbspdisplay.c')
$spDisplaySlice = (Get-CPrologue $spDisplaySource 'HSD_LObj* lb_80011AC4(') +
    (Get-CFunction $spDisplaySource 'HSD_LObj* lb_80011AC4(') + "`r`n" +
    (Get-CFunction $spDisplaySource 'int lb_80011E24(') + "`r`n" +
    (Get-CFunction $spDisplaySource 'int lb_8001204C(') + "`r`n"
Set-Content -Encoding ASCII (Join-Path $build 'lbspdisplay_joint_slice.c') $spDisplaySlice
$lbJointSource = Get-Content -Raw (Join-Path $meleeSrc 'melee/lb/lb_00B0.c')
$lbJointSlice = (Get-CPrologue $lbJointSource 'static ') +
    (Get-CFunction $lbJointSource 'void lb_8000B1CC(') + "`r`n"
Set-Content -Encoding ASCII (Join-Path $build 'lb_00B0_slice.c') $lbJointSlice
$sceneObjects = @()
$sceneSources = @(
    @{ Source = (Join-Path $baselibDir 'cobj.c'); Dir = $baselibDir },
    @{ Source = (Join-Path $baselibDir 'fog.c'); Dir = $baselibDir },
    @{ Source = (Join-Path $baselibDir 'lobj.c'); Dir = $baselibDir },
    @{ Source = (Join-Path $build 'displayfunc_matrix_slice.c'); Dir = $baselibDir },
    @{ Source = (Join-Path $build 'lbspdisplay_joint_slice.c'); Dir = (Join-Path $meleeSrc 'melee/lb') },
    @{ Source = (Join-Path $build 'lb_00B0_slice.c'); Dir = (Join-Path $meleeSrc 'melee/lb') },
    @{ Source = (Join-Path $meleeSrc 'melee/mn/mnmain.c'); Dir = (Join-Path $meleeSrc 'melee/mn') },
    @{ Source = (Join-Path $meleeSrc 'melee/mn/mn_22EC.c'); Dir = (Join-Path $meleeSrc 'melee/mn') },
    @{ Source = $menuSceneSource; Dir = (Join-Path $root 'src/xdk') },
    @{ Source = (Join-Path $root 'src/xdk/match_scene_xdk.c'); Dir = (Join-Path $root 'src/xdk') },
    @{ Source = (Join-Path $root 'src/xdk/hsd_gx_xdk.c'); Dir = (Join-Path $root 'src/xdk') }
)
Write-Host '[M360][XEX] compiling original camera/fog/light/menu scene code'
foreach ($scene in $sceneSources) {
    $sceneObject = Join-Path $build ([IO.Path]::GetFileNameWithoutExtension($scene.Source) + '_scene.obj')
    $level = if ($scene.Source.StartsWith((Join-Path $root 'src'))) { '/W4' } else { '/W3' }
    & $compiler (@('/nologo', '/c', '/TC', '/O2', '/MT', '/GS-', $level,
        '/D_XBOX', '/DXBOX', '/DNDEBUG',
        "/I$(Join-Path $root 'build-x360/gameplay-probe/include')", "/I$($scene.Dir)",
        "/I$(Join-Path $root 'src/xdk')", "/FI$sceneCompat", "/Fo$sceneObject",
        $scene.Source) + $hsdCommonInc)
    if ($LASTEXITCODE -ne 0) { throw "Scene source compilation failed: $($scene.Source)" }
    $sceneObjects += $sceneObject
}
Write-Host '[M360][XEX] compiling original fighter states and native match glue'
$matchObjects = @(& (Join-Path $PSScriptRoot 'build_match_xdk.ps1') -Compiler $compiler -Build $build)
$linkArgs = @(
    '/NOLOGO', '/MACHINE:PPCBE', '/SUBSYSTEM:XBOX', '/XEX:NO',
    '/INCREMENTAL:NO', '/OPT:REF', "/OUT:$pe", "/PDB:$pdb", "/MAP:$(Join-Path $build 'melee360.map')", "/LIBPATH:$libXbox",
    $gameplayLayoutObject, $menuInputObject, $menuInputTestObject,
    $object, $compatObject, $lbtimeObject, $padObject, $controllerObject,
    $lbmathObject, $spriteObject, $bootObject, $archiveObject,
    $titleSceneObject, $hsdRenderObject, $hsdTextureObject, $gcmObject,
    $memoryObject, $memoryWrapperObject,
    $hsdClassWrapperObject, $hashObject, $debugObject, $classObject,
    $objectObject, $objallocObject, $idObject,
    $gobjWrapperObject, $listObject, $gobjObjectObject, $gobjUserDataObject,
    $gobjProcObject, $gobjPLinkObject, $gobjGXLinkObject, $gobjInitObject,
    $gobjObject,
    $hsdMathWrapperObject, $mtxObject, $quatlibObject, $splineObject,
    $fobjObject, $randomObject,
    $hsdAnimWrapperObject, $aobjObject, $dobjObject, $robjObject,
    $utilObject, $bytecodeObject,
    $hsdJObjWrapperObject, $jobjObject, $wobjObject,
    $hsdSynthWrapperObject, $synthObject, $devcomObject
) + $sceneObjects + $matchObjects + $audioObjects + $movieObjects + @(
    'xaudio2.lib', 'xmcore.lib',
    'd3d9.lib', 'd3dx9.lib', 'xapilib.lib', 'xboxkrnl.lib'
)
$linkResponse = Join-Path $build 'link.rsp'
Set-Content -Encoding ASCII $linkResponse ($linkArgs | ForEach-Object { '"' + $_ + '"' })
& $linker "@$linkResponse"
if ($LASTEXITCODE -ne 0) { throw 'XDK PE link failed.' }

Write-Host '[M360][XEX] building dist/default.xex'
& $imagexex "/IN:$pe" "/OUT:$xex"
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $xex)) {
    throw 'imagexex failed.'
}

$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $xex).Hash
Write-Host "[M360][XEX] created $xex"
Write-Host "[M360][XEX] SHA256=$hash"
