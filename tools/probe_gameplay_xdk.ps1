[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$compiler = Join-Path $env:XEDK 'bin/win32/cl.exe'
if (!(Test-Path $compiler)) { throw 'Xbox compiler unavailable.' }
$output = Join-Path $root 'build-x360/gameplay-probe'
New-Item -ItemType Directory -Force $output | Out-Null
$overlay = Join-Path $output 'include'
 $headers = & rg -l 'static MotionFlags const' "$root/upstream/melee-pc/src/melee/ft" -g '*.h'
foreach ($path in $headers) {
    $header = $path.Substring(("$root/upstream/melee-pc/src/").Length)
    $text = Get-Content -Raw (Join-Path "$root/upstream/melee-pc/src" $header)
    # These integral flags must be constant expressions in the XDK C frontend.
    $text = [regex]::Replace($text, 'static MotionFlags const (\w+)\s*=\s*([^;]+);', 'enum { $1 = $2 };')
    $destination = Join-Path $overlay $header
    New-Item -ItemType Directory -Force (Split-Path $destination) | Out-Null
    Set-Content -Encoding ASCII $destination $text
}
$ground = Get-Content -Raw "$root/upstream/melee-pc/src/melee/gr/types.h"
# VC's C offsetof rejects indexed members; retain the same byte comparison.
$ground = [regex]::Replace($ground, 'offsetof\(struct (\w+), (\w+)\[(\d+)\]\)', '(offsetof(struct $1, $2) + $3 * sizeof(((struct $1*)0)->$2[0]))')
# The indexed member is passed through GRCASTLE_ALIAS, so normalize its calls.
$ground = [regex]::Replace($ground, 'GRCASTLE_ALIAS\((\w+), (\w+), (\w+), (\w+)\[(\d+)\]\);', 'STATIC_ASSERT(offsetof(struct $1, $2) == offsetof(struct $3, $4) + $5 * sizeof(((struct $3*)0)->$4[0]));')
New-Item -ItemType Directory -Force "$overlay/melee/gr" | Out-Null
$ground = $ground.Replace('GRCASTLE_BELOW(grCastle_GroundVars8, plat[0].state, grCastle_GroundVars7, xD0);', 'STATIC_ASSERT(offsetof(struct grCastle_GroundVars8, plat) + offsetof(struct grCastle_Platform, state) + sizeof(((struct grCastle_Platform*)0)->state) <= offsetof(struct grCastle_GroundVars7, xD0));')
Set-Content -Encoding ASCII "$overlay/melee/gr/types.h" $ground
$sources = @('melee/ft/fighter.c', 'melee/ft/kinds/ftCommon/ftCo_Wait.c', 'melee/gm/gmtitle.c', 'melee/mn/mn_22EC.c')
$failed = 0
foreach ($source in $sources) {
    $name = [IO.Path]::GetFileNameWithoutExtension($source)
    $language = '/TC'
    $arguments = @('/nologo','/c',$language,'/O2','/D_XBOX','/DXBOX','/DNDEBUG',
        "/I$overlay", "/I$env:XEDK/include/xbox", "/I$root/src/xdk",
        "/I$root/upstream/melee-pc/src", "/I$root/upstream/melee-pc/src/sdk_include",
        "/FI$root/src/xdk/gameplay_probe_compat.h", "/Fo$output/$name.obj",
        "$root/upstream/melee-pc/src/$source")
    $lines = & $compiler @arguments
    $result = $LASTEXITCODE
    $lines | Out-File -Encoding utf8 "$output/$name.log"
    Write-Output "$source exit=$result"
    $lines | Select-String 'error ' | Select-Object -First 8
    if ($result -ne 0) { ++$failed }
}
if ($failed) { throw "$failed gameplay compilation probes failed; see $output" }
