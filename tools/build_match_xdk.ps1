[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string] $Compiler,
    [Parameter(Mandatory = $true)] [string] $Build
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$src = Join-Path $root 'upstream/melee-pc/src'
$overlay = Join-Path $root 'build-x360/gameplay-probe/include'
$probe = Join-Path $root 'build-x360/gameplay-probe'
$out = Join-Path $Build 'match'
New-Item -ItemType Directory -Force $out | Out-Null

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

function New-Slice([string]$Relative, [string]$FirstSignature, [string[]]$Signatures, [string]$Name) {
    $text = Get-Content -Raw (Join-Path $src $Relative)
    $end = $text.IndexOf($FirstSignature)
    if ($end -lt 0) { throw "Missing original function: $FirstSignature" }
    $slice = $text.Substring(0, $end)
    foreach ($signature in $Signatures) { $slice += (Get-CFunction $text $signature) + "`r`n" }
    $path = Join-Path $out "$Name.c"
    Set-Content -Encoding ASCII $path $slice
    return @{ Path = $path; Dir = (Split-Path (Join-Path $src $Relative)) }
}

# Original motion-state entries, copied verbatim from ftmotionstates.c.
$motionIds = @(14,15,16,17,18,19,20,21,23,24,25,26,27,28,29,30,31,32,33,34,42,43,44,45,46)
$motionText = Get-Content -Raw (Join-Path $src 'melee/ft/ftmotionstates.c')
$tableStart = $motionText.IndexOf('MotionState ftData_MotionStateList[ftCo_MS_Count] = {')
$headers = $motionText.Substring(0, $tableStart)
$entries = [regex]::Matches($motionText.Substring($tableStart), '(?s)\n    \{\s*\n\s*// ftCo_MS_(\w+) = (\d+)\s*\n(.*?)\n    \},')
$table = $headers + "`r`ntypedef struct M360MotionEntry { int msid; MotionState state; } M360MotionEntry;`r`n"
$table += "const M360MotionEntry M360_MotionTable[] = {`r`n"
$found = 0
foreach ($entry in $entries) {
    $id = [int]$entry.Groups[2].Value
    if ($motionIds -notcontains $id) { continue }
    $table += "    { $id, {`r`n        // ftCo_MS_$($entry.Groups[1].Value)`r`n$($entry.Groups[3].Value)`r`n    } },`r`n"
    ++$found
}
if ($found -ne $motionIds.Count) { throw "Motion table extraction found $found of $($motionIds.Count) entries." }
$table += "};`r`nconst unsigned M360_MotionTableCount = $found;`r`n"
$motionTable = Join-Path $out 'motion_table.c'
Set-Content -Encoding ASCII $motionTable $table

$units = @(
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Wait.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Walk.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Dash.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Run.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_RunBrake.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Turn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_TurnRun.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_KneeBend.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Jump.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_JumpAerial.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Fall.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_FallAerial.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Landing.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Attack1.c') },
    @{ Path = (Join-Path $src 'melee/ft/ftwalkcommon.c') },
    @{ Path = (Join-Path $src 'melee/ft/ftaction.c') },
    @{ Path = (Join-Path $src 'melee/ft/ftcommon.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_084E.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_08A1.c') },
    @{ Path = (Join-Path $src 'melee/ft/ftwaitanim.c') },
    @{ Path = (Join-Path $probe 'ftchangeparam.c'); Dir = (Join-Path $src 'melee/ft') },
    @{ Path = (Join-Path $src 'melee/lb/lbcommand.c') },
    @{ Path = (Join-Path $src 'melee/lb/lbanim.c') },
    @{ Path = $motionTable; Dir = (Join-Path $src 'melee/ft') }
)
$units += New-Slice 'melee/ft/ft_081B.c' 'void ft_80081B38(' @('void ft_80082B1C(', 'void ft_80084DB0(') 'ft_081B_slice'
$units += New-Slice 'melee/ft/ftswing.c' 'void ftCo_FallAerial_Coll(' @('void ftCo_FallAerial_Coll(') 'ftswing_slice'
$units += New-Slice 'melee/ft/kinds/ftCommon/ftCo_FallSpecial.c' 'void ftCo_800968C8(' @('bool ftCo_80096CC8(') 'ftCo_FallSpecial_slice'

$objects = @()
$base = @('/nologo','/c','/TC','/O2','/MT','/GS-','/D_XBOX','/DXBOX','/DNDEBUG',
    "/I$overlay", "/I$env:XEDK/include/xbox", "/I$(Join-Path $root 'src/xdk')",
    "/I$src", "/I$(Join-Path $src 'sdk_include')",
    "/FI$(Join-Path $root 'src/xdk/gameplay_probe_compat.h')",
    "/FI$overlay/melee/ft/forward.h", "/FI$overlay/melee/ft/kinds/ftCommon/forward.h",
    "/FI$overlay/melee/ft/types.h")
foreach ($unit in $units) {
    $dir = if ($unit.Dir) { $unit.Dir } else { Split-Path $unit.Path }
    $object = Join-Path $out ([IO.Path]::GetFileNameWithoutExtension($unit.Path) + '.obj')
    & $Compiler ($base + @('/W3', "/I$dir", "/Fo$object", $unit.Path)) | Write-Host
    if ($LASTEXITCODE -ne 0) { throw "Original fighter unit failed: $($unit.Path)" }
    $objects += $object
}
$nativeBase = @('/nologo','/c','/TC','/O2','/MT','/GS-','/W4','/D_XBOX','/DXBOX','/DNDEBUG',
    "/I$overlay", "/I$env:XEDK/include/xbox", "/I$(Join-Path $root 'src/xdk')",
    "/I$src", "/I$(Join-Path $src 'sdk_include')",
    "/FI$(Join-Path $root 'src/xdk/fighter_glue_compat.h')")
foreach ($native in @('fighter_glue_xdk.c', 'fighter_unported_xdk.c')) {
    $object = Join-Path $out ([IO.Path]::GetFileNameWithoutExtension($native) + '.obj')
    & $Compiler ($nativeBase + @("/Fo$object", (Join-Path $root "src/xdk/$native"))) | Write-Host
    if ($LASTEXITCODE -ne 0) { throw "Fighter glue failed: $native" }
    $objects += $object
}
return $objects
