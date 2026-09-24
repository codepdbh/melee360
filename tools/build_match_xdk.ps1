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
$matchOverlay = Join-Path $out 'include'
New-Item -ItemType Directory -Force (Join-Path $matchOverlay 'pc') | Out-Null
$disc = (Get-Content -Raw (Join-Path $src 'pc/disc.h')).Replace('#define DP(T, slot) (slot)', '#define DP(T, slot) ((T*) (uintptr_t) (slot))')
Set-Content -Encoding ASCII (Join-Path $matchOverlay 'pc/disc.h') $disc
New-Item -ItemType Directory -Force (Join-Path $matchOverlay 'melee/lb') | Out-Null
$lbTypes = Get-Content -Raw (Join-Path $src 'melee/lb/types.h')
$skipPattern = '(?s)(struct DISC_STRUCT spawn_hitbox_skip \{\s*u8 _0\[0xF\];)(.*?)(\};)'
if ($lbTypes -notmatch $skipPattern) { throw 'spawn_hitbox_skip layout not found' }
$lbTypes = [regex]::Replace($lbTypes, $skipPattern, { param($m) $m.Groups[1].Value + $m.Groups[2].Value.Replace('u32 xF_', 'u8 xF_') + $m.Groups[3].Value })
Set-Content -Encoding ASCII (Join-Path $matchOverlay 'melee/lb/types.h') $lbTypes

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

function New-RangeSlice([string]$Relative, [string]$FirstSignature, [string]$StartSignature, [string]$EndSignature, [string]$Name) {
    $text = Get-Content -Raw (Join-Path $src $Relative)
    $end = $text.IndexOf($FirstSignature)
    $start = $text.IndexOf($StartSignature)
    if ($end -lt 0 -or $start -lt 0) { throw "Missing original range in $Relative" }
    $last = Get-CFunction $text $EndSignature
    $stop = $text.IndexOf($last) + $last.Length
    $path = Join-Path $out "$Name.c"
    Set-Content -Encoding ASCII $path ($text.Substring(0, $end) + $text.Substring($start, $stop - $start) + "`r`n")
    return @{ Path = $path; Dir = (Split-Path (Join-Path $src $Relative)) }
}

# Original motion-state entries, copied verbatim from ftmotionstates.c.
$motionIds = @(14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,247,248,249,250)
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
    $body = $entry.Groups[3].Value
    $table += "    { $id, {`r`n        // ftCo_MS_$($entry.Groups[1].Value)`r`n$body`r`n    } },`r`n"
    ++$found
}
if ($found -ne $motionIds.Count) { throw "Motion table extraction found $found of $($motionIds.Count) entries." }
$table += "};`r`nconst unsigned M360_MotionTableCount = $found;`r`n"
$motionTable = Join-Path $out 'motion_table.c'
Set-Content -Encoding ASCII $motionTable $table

$fighterSource = Get-Content -Raw (Join-Path $src 'melee/ft/fighter.c')
$fighterConsts = [regex]::Match($fighterSource, '(?s)const Vec3 Fighter_803B7488 = [^;]*;\s*const Vec3 vec3_803B7494 = [^;]*;').Value
$rangeStart = $fighterSource.IndexOf('void Fighter_8006A1BC(')
$rangeLast = Get-CFunction $fighterSource 'void Fighter_ProcessHit_8006D1EC('
$rangeStop = $fighterSource.IndexOf($rangeLast) + $rangeLast.Length
$globalsStart = $fighterSource.IndexOf('HSD_ObjAllocData fighter_alloc_data;')
$globalsEnd = $fighterSource.IndexOf('ftCommonData* p_ftCommonData;') + 'ftCommonData* p_ftCommonData;'.Length
$fighterSpawn = $fighterSource.Substring($globalsStart, $globalsEnd - $globalsStart) + "`r`n" +
    (Get-CFunction $fighterSource 'void Fighter_LoadCommonData(')
$fighterSlice = (Get-CPrologue $fighterSource 'const Vec3 Fighter_803B7488') + $fighterConsts + "`r`n" + $fighterSpawn + "`r`n" +
    (Get-CFunction $fighterSource 'void Fighter_UnkInitReset_80067C98(') + "`r`n" +
    (Get-CFunction $fighterSource 'void Fighter_ResetInputData_80068854(') + "`r`n" +
    (Get-CFunction $fighterSource 'static void Fighter_UnkInitLoad_80068914_Inner1(') + "`r`n" +
    (Get-CFunction $fighterSource 'u32 Fighter_NewSpawn_80068E40(') + "`r`n" +
    (Get-CFunction $fighterSource 'void Fighter_ChangeMotionState(') + "`r`n" +
    $fighterSource.Substring($rangeStart, $rangeStop - $rangeStart) + "`r`n"
$fighterSlicePath = Join-Path $out 'fighter_frame_slice.c'
Set-Content -Encoding ASCII $fighterSlicePath $fighterSlice

$collSource = Get-Content -Raw (Join-Path $src 'melee/lb/lbcollision.c')
$collStart = $collSource.IndexOf('/* 006E58 */ static bool')
$collLast = Get-CFunction $collSource 'void lbColl_80008A5C('
$collStop = $collSource.IndexOf($collLast) + $collLast.Length
$collSlicePath = Join-Path $out 'lbcollision_slice.c'
Set-Content -Encoding ASCII $collSlicePath ($collSource.Substring(0, $collStop) + "`r`n" + (Get-CFunction $collSource 'void lbColl_80008D30(') + "`r`n" + (Get-CFunction $collSource 'bool lbColl_8000ACFC(') + "`r`n")

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
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Damage.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_DamageFall.c') },
    @{ Path = (Join-Path $src 'melee/ft/ftcoll.c') },
    @{ Path = $collSlicePath; Dir = (Join-Path $src 'melee/lb') },
    @{ Path = (Join-Path $src 'melee/lb/lbvector.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_07C1.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_07C6.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_0819.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_0892.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_0C88.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_0C8C.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_0DF0.c') },
    @{ Path = (Join-Path $src 'melee/ft/ftcolanim.c') },
    @{ Path = (Join-Path $src 'melee/ft/ftdevice.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_0C35.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_0D67.c') },
    @{ Path = (Join-Path $src 'melee/pl/plstale.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_0D8E.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_0D95.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_0DC2.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CaptureCut.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CaptureJump.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CapturePulled.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CaptureWait.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Catch.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CatchAttack.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CatchCut.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CatchPull.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CatchWait.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Escape.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_EscapeAir.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Furafura.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Guard.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Rebound.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ShieldBreakDown.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ShieldBreakFall.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ShieldBreakFly.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ShieldBreakStand.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Throw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Thrown.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_DownAttack.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_DownBound.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_DownDamage.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_DownSpot.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_DownStand.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Down.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_FlyReflect.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_PassiveCeil.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_PassiveStand.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_PassiveWall.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Passive.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_StopCeil.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_StopWall.c') },
    @{ Path = $fighterSlicePath; Dir = (Join-Path $src 'melee/ft') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Attack1.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_AttackS3.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_AttackHi3.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_AttackLw3.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_AttackS4.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_AttackHi4.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_AttackLw4.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_AttackAir.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_LandingAir.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_AttackDash.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Attack100.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Squat.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_SquatWait.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_SquatRv.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_RunDirect.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Pass.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_0DF1.c') },
    @{ Path = (Join-Path $src 'melee/ft/ftattacks4combo.c') },
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
$units += New-Slice 'melee/ft/kinds/ftCommon/ftCo_0A01.c' '/// @todo .sdata2 order hack' @('static inline float convertStickAxis(', 'float ftCo_GetCpuLStickX(', 'float ftCo_GetCpuLStickY(', 'float ftCo_GetCpuLTrigger(', 'float ftCo_GetCpuRTrigger(', 'HSD_Pad ftCo_GetCpuButtons(', 'float ftCo_GetCpuCStickX(', 'float ftCo_GetCpuCStickY(', 'bool ftCo_IsCpuControlled(') 'ftCo_0A01_cpu_input_slice'
$units += New-Slice 'melee/ft/ft_0881.c' 'void ft_800881D8(' @('void ft_800890BC(', 'static inline void inlineB0(', 'void ft_800890D0(', 'static f32 ft_80089118(', 'f32 ft_80089228(', 'static inline void inlineC0(', 'void ft_800892A0(') 'ft_0881_stale_slice'
$units += New-Slice 'melee/ft/ftswing.c' 'void ftCo_FallAerial_Coll(' @('void ftCo_FallAerial_Coll(') 'ftswing_slice'
$units += New-Slice 'melee/ft/kinds/ftCommon/ftCo_FallSpecial.c' 'void ftCo_800968C8(' @('bool ftCo_80096CC8(') 'ftCo_FallSpecial_slice'

$objects = @()
$base = @('/nologo','/c','/TC','/O2','/MT','/GS-','/D_XBOX','/DXBOX','/DNDEBUG',
    "/I$matchOverlay", "/I$overlay", "/I$env:XEDK/include/xbox", "/I$(Join-Path $root 'src/xdk')",
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
    "/I$matchOverlay", "/I$overlay", "/I$env:XEDK/include/xbox", "/I$(Join-Path $root 'src/xdk')",
    "/I$src", "/I$(Join-Path $src 'sdk_include')",
    "/FI$(Join-Path $root 'src/xdk/fighter_glue_compat.h')")
foreach ($native in @('fighter_glue_xdk.c', 'fighter_unported_xdk.c')) {
    $object = Join-Path $out ([IO.Path]::GetFileNameWithoutExtension($native) + '.obj')
    & $Compiler ($nativeBase + @("/Fo$object", (Join-Path $root "src/xdk/$native"))) | Write-Host
    if ($LASTEXITCODE -ne 0) { throw "Fighter glue failed: $native" }
    $objects += $object
}
return $objects
