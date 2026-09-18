[CmdletBinding()]
param()

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
$spriteSource = Join-Path $root 'src\xdk\sprite_renderer.cpp'
$spriteHeader = Join-Path $root 'src\xdk\sprite_renderer.h'
$bootSource = Join-Path $root 'src\xdk\melee_boot_xdk.cpp'
$bootHeader = Join-Path $root 'src\xdk\melee_boot_xdk.h'
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
                         $hsdJObjWrapper, $jobjSource, $wobjSource)) {
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

$gcmArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/GS-', '/W4', '/TC',
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys", "/Fo$gcmObject", $gcmSource
)
& $compiler $gcmArgs
if ($LASTEXITCODE -ne 0) { throw 'GameCube FST reader compilation failed.' }

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
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
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
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
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
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
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

Write-Host '[M360][XEX] linking Xbox 360 PowerPC PE'
$linkArgs = @(
    '/NOLOGO', '/MACHINE:PPCBE', '/SUBSYSTEM:XBOX', '/XEX:NO',
    '/INCREMENTAL:NO', "/OUT:$pe", "/PDB:$pdb", "/LIBPATH:$libXbox",
    $object, $compatObject, $lbtimeObject, $padObject, $controllerObject,
    $lbmathObject, $spriteObject, $bootObject, $gcmObject,
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
    'd3d9.lib', 'd3dx9.lib', 'xapilib.lib', 'xboxkrnl.lib'
)
& $linker $linkArgs
if ($LASTEXITCODE -ne 0) { throw 'XDK PE link failed.' }

Write-Host '[M360][XEX] building dist/default.xex'
& $imagexex "/IN:$pe" "/OUT:$xex"
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $xex)) {
    throw 'imagexex failed.'
}

$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $xex).Hash
Write-Host "[M360][XEX] created $xex"
Write-Host "[M360][XEX] SHA256=$hash"
