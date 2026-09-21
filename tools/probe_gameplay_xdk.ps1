[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$compiler = Join-Path $env:XEDK 'bin/win32/cl.exe'
if (!(Test-Path $compiler)) { throw 'Xbox compiler unavailable.' }
$output = Join-Path $root 'build-x360/gameplay-probe'
New-Item -ItemType Directory -Force $output | Out-Null
$sources = @('melee/ft/fighter.c', 'melee/ft/kinds/ftCommon/ftCo_Wait.c', 'melee/gm/gmtitle.c', 'melee/mn/mn_22EC.c')
$failed = 0
foreach ($source in $sources) {
    $name = [IO.Path]::GetFileNameWithoutExtension($source)
    $language = if ($source -like 'melee/ft/*') { '/TP' } else { '/TC' }
    $arguments = @('/nologo','/c',$language,'/O2','/D_XBOX','/DXBOX','/DNDEBUG',
        "/I$env:XEDK/include/xbox", "/I$root/src/xdk",
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
