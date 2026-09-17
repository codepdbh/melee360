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
$build = Join-Path $root 'build-x360\xdk'
$dist = Join-Path $root 'dist'
$object = Join-Path $build 'main.obj'
$compatObject = Join-Path $build 'melee_time_compat.obj'
$lbtimeObject = Join-Path $build 'lbtime.obj'
$padObject = Join-Path $build 'melee_pad_xdk.obj'
$controllerObject = Join-Path $build 'controller.obj'
$pe = Join-Path $build 'melee360.exe'
$pdb = Join-Path $build 'melee360.pdb'
$xex = Join-Path $dist 'default.xex'

foreach ($required in @($compiler, $linker, $imagexex,
                         (Join-Path $includeXbox 'xtl.h'),
                         (Join-Path $libXbox 'xboxkrnl.lib'),
                         $source, $compatSource, $compatHeader,
                         $lbtimeSource, $padSource, $padHeader,
                         $controllerCompat, $controllerSource,
                         $meleeSdkInclude)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required XDK component is missing: $required"
    }
}

New-Item -ItemType Directory -Path $build, $dist -Force | Out-Null

Write-Host '[M360][XEX] compiling PowerPC source'
$compileArgs = @(
    '/nologo', '/c', '/O2', '/MT', '/EHsc-', '/GR-', '/GS-', '/W4',
    '/D_XBOX', '/DXBOX', '/DNDEBUG',
    "/I$includeXbox", "/I$includeSys", "/Fo$object", $source
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

Write-Host '[M360][XEX] linking Xbox 360 PowerPC PE'
$linkArgs = @(
    '/NOLOGO', '/MACHINE:PPCBE', '/SUBSYSTEM:XBOX', '/XEX:NO',
    '/INCREMENTAL:NO', "/OUT:$pe", "/PDB:$pdb", "/LIBPATH:$libXbox",
    $object, $compatObject, $lbtimeObject, $padObject, $controllerObject,
    'd3d9.lib', 'xapilib.lib', 'xboxkrnl.lib'
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
