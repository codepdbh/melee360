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

# Drop unused stack-padding arrays inside function bodies (they only steer the
# original codegen and are often C99 mixed declarations the C89 XDK frontend
# rejects); struct/union members are untouched. Line numbers are preserved.
function Convert-PaddingDecls([string]$Text) {
    $padLine = '^(\s*)((?:UNUSED\s+)?(?:unsigned char|u8|s8|char|u16|s16|u32|s32|int|u64|f32|float)\s+_\s*\[[^\]]*\]\s*;)(\s*)$'
    $lines = $Text -split "`n"
    $stack = New-Object System.Collections.Generic.List[string]
    $pending = ''
    $changed = $false
    for ($i = 0; $i -lt $lines.Count; ++$i) {
        $line = $lines[$i]
        $trimmed = $line.TrimStart()
        if ($trimmed.StartsWith('#')) { continue }
        if ($stack.Count -gt 0 -and $stack[$stack.Count - 1] -eq 'code') {
            $m = [regex]::Match($line.TrimEnd("`r"), $padLine)
            if (-not $m.Success) { $m = [regex]::Match($line.TrimEnd("`r"), '^\s*_\s*\[\s*\d+\s*\]\s*=\s*[^;]+;\s*$') }
            if ($m.Success) {
                $lines[$i] = $(if ($line.EndsWith("`r")) { "`r" } else { '' })
                $changed = $true
                $pending = ''
                continue
            }
        }
        $code = [regex]::Replace($line, '//.*$|"(?:\\.|[^"\\])*"|''(?:\\.|[^''\\])*''', '')
        foreach ($ch in $code.ToCharArray()) {
            if ($ch -eq '{') {
                $kind = if ($pending -match '\b(struct|union|enum)\b|=\s*$|=\s*\{?\s*$') { 'data' } elseif ($stack.Count -gt 0 -and $stack[$stack.Count - 1] -eq 'data') { 'data' } else { 'code' }
                $stack.Add($kind); $pending = ''
            } elseif ($ch -eq '}') {
                if ($stack.Count -gt 0) { $stack.RemoveAt($stack.Count - 1) }
                $pending = ''
            } elseif ($ch -eq ';') {
                $pending = ''
            } else {
                $pending += $ch
            }
        }
        $pending += ' '
    }
    if ($changed) { return ($lines -join "`n") }
    return $Text
}

function New-Adapted([string]$Relative, [hashtable]$Replacements, [string]$Name) {
    $text = Get-Content -Raw (Join-Path $src $Relative)
    foreach ($key in $Replacements.Keys) { $text = [regex]::Replace($text, $key, $Replacements[$key]) }
    $path = Join-Path $out "$Name.c"
    Set-Content -Encoding ASCII $path $text
    return @{ Path = $path; Dir = (Split-Path (Join-Path $src $Relative)) }
}

# Original motion-state entries, copied verbatim from ftmotionstates.c.
$motionIds = @(14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,247,248,249,250,244,178,179,180,181,182,205,206,207,208,209,210,211,233,234,235,236,237,238,35,36,37,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,239,240,241,242,243,252,253,254,255,256,257,258,259,260,261,262,263,264,265,245,246,12,13,266,267,268,269,270,301,302,303,304,144,145,146,147,276,277,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,307,308,309,310,311,312,313,314,315,316,317,318,319,320,321,294,295,296,297,298,299,300,305,306,325,326,340,293,288,289,290,291,292,331,332)
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
    @{ Path = $motionTable; Dir = (Join-Path $src 'melee/ft') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_FallSpecial.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Guard.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Escape.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_EscapeAir.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ShieldBreakFly.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ShieldBreakFall.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ShieldBreakDown.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ShieldBreakStand.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Furafura.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Rebound.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Catch.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CatchPull.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CatchWait.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CatchAttack.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CatchCut.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Throw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CapturePulled.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CaptureWait.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CaptureCut.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CaptureJump.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Thrown.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_0D8E.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_0D95.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_0DC2.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_SpecialS.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_SpecialAir.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftMario/ftmario.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftMario/ftmariospecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftMario/ftmariospecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftMario/ftmariospecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftMario/ftmariospeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftMario/ftmariostrings.c') },
    @{ Path = (Join-Path $src 'melee/ft/ftcliffcommon.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CliffWait.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CliffClimb.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CliffAttack.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CliffEscape.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CliffJump.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_AppealS.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Ottotto.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_0D4D.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftLuigi/ftluigi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftLuigi/ftluigispecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftLuigi/ftluigispecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftLuigi/ftluigispecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftLuigi/ftluigispeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftDrMario/ftdrmario.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftDrMario/ftdrmarioappeals.c') },
    @{ Path = (Join-Path $src 'melee/ft/ftswing.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CargoFall.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CargoJump.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CargoKneebend.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CargoLanding.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CargoThrow.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CargoTurn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CargoWait.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CargoWalk.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_JumpAerialF1.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftpickupitem.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ItemThrow.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Lift.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ItemScrew.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ItemScope.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ItemScopeFire.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ItemScopeRapid.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ItemScopeStart.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_DamageScrew.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_HammerFall.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_HammerJump.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_HammerKneeBend.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_HammerLanding.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_HammerTurn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_HammerWait.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_HammerWalk.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_KinokoGiantEnd.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_KinokoGiantStart.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_KinokoSmallEnd.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_KinokoSmallStart.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_WarpStar.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Barrel.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_BarrelWait.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Bury.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_BuryWait.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_DamageBind.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_DamageSong.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_DamageIce.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_0CD1.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_0CD3.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_0CDD.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_0CDF.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_0CE3.c') },
    @{ Path = (Join-Path $src 'melee/ft/ftlipstickswing.c') },
    @{ Path = (Join-Path $src 'melee/ft/ftstarrodswing.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_0D27.c') },
    @{ Path = (Join-Path $src 'sysdolphin/baselib/particle.c') },
    @{ Path = (Join-Path $src 'sysdolphin/baselib/generator.c') },
    @{ Path = (Join-Path $src 'sysdolphin/baselib/psappsrt.c') },
    @{ Path = (Join-Path $src 'melee/ef/efalt.c') },
    @{ Path = (Join-Path $src 'melee/ef/efdata.c') },
    @{ Path = (Join-Path $src 'melee/ef/eflib.c') },
    @{ Path = (Join-Path $src 'melee/ef/efsync.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_0A01.c') },
    @{ Path = (Join-Path $src 'melee/ft/ftcmdscript.c') },
    @{ Path = (Join-Path $src 'melee/ft/ftcpuattack.c') },
    @{ Path = (Join-Path $src 'melee/ft/ft_3C61.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPopo/ftpopo.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPopo/ftpopospecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPopo/ftpopospeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPopo/ftpopospecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPopo/ftpopospecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftNana/ftnana.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftNana/ftnanaspecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftNana/ftnanaspecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CaptureKirby.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CaptureWaitKirby.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ThrownKirby.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirby.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyattackdash.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbycaptureyoshi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbydata.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialcaptain.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialdonkey.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialfox.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialgamewatch.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialiceclimber.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialkoopa.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspeciallink.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialluigi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialmario.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialmars.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialmewtwo.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialness.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialpeach.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialpikachu.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialpurin.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialsamus.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialseak.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialyoshi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyspecialzelda.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKirby/ftkirbyyoshiegg.c') },
    @{ Path = (Join-Path $src 'melee/it/it_26B1.c') },
    @{ Path = (Join-Path $src 'melee/it/it_2725.c') },
    @{ Path = (Join-Path $src 'melee/it/it_279C.c') },
    @{ Path = (Join-Path $src 'melee/it/it_3F14.c') },
    @{ Path = (Join-Path $src 'melee/it/it_3F2F.c') },
    @{ Path = (Join-Path $src 'melee/it/itanimlist.c') },
    @{ Path = (Join-Path $src 'melee/it/itcoll.c') },
    @{ Path = (Join-Path $src 'melee/it/itdraw.c') },
    @{ Path = (Join-Path $src 'melee/it/itdrop.c') },
    @{ Path = (Join-Path $src 'melee/it/iteffect.c') },
    @{ Path = (Join-Path $src 'melee/it/item.c') },
    @{ Path = (Join-Path $src 'melee/it/itgroundcoll.c') },
    @{ Path = (Join-Path $src 'melee/it/ithitbox.c') },
    @{ Path = (Join-Path $src 'melee/it/itmaplib.c') },
    @{ Path = (Join-Path $src 'melee/it/itmaterial.c') },
    @{ Path = (Join-Path $src 'melee/it/itspawn.c') },
    @{ Path = (Join-Path $src 'melee/ft/ftlib.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itmariofireball.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/it_2ADA.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/it_2E5A.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/it_2F28.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itarwinglaser.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itbat.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itbombhei.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itbox.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itcapsule.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itcerebi.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itchicorita.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itclimbersblizzard.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itclimbersice.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itclimbersstring.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itclinkmilk.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itcoin.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itcrazyhandbomb.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itdkinoko.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itdosei.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itdrmariopill.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itegg.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itentei.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itevyoshiegg.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itfflower.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itfflowerflame.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itfire.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itflipper.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itfoods.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itfoxblaster.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itfoxillusion.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itfoxlaser.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itfreeze.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itfreezer.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itfushigibana.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itgamewatchbreath.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itgamewatchchef.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itgamewatchfire.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itgamewatchgreenhouse.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itgamewatchjudge.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itgamewatchmanhole.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itgamewatchpanic.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itgamewatchparachute.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itgamewatchrescue.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itgamewatchturtle.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itgreatfoxlaser.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itgshell.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ithammer.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ithammerhead.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itharisen.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ithassam.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itheart.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itheiho.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ithinoarashi.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ithitodeman.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ithouou.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itkabigon.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itkamex.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itkinoko.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itkirby_2F23.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itkirbycutterbeam.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itkirbygamewatchchefpan.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itkirbyhammer.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itkirbyyoshispecialn.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itkireihana.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itklap.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itkoopaflame.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itkusudama.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itkyasarin.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itkyasarinegg.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itleadead.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itlgun.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itlgunbeam.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itlgunray.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itlikelike.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itlinkarrow.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itlinkbomb.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itlinkboomerang.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itlinkbow.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itlinkhookshot.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itlipstick.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itlipstickspore.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itlizardon.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itlucky.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itlugia.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itluigifireball.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itmaril.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itmariocape.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itmarumine.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itmasterhandbullet.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itmasterhandlaser.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itmatadogas.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itmato.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itmball.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itmetalb.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itmetamon.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itmew.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itmewtwodisable.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itmewtwoshadowball.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itmsbomb.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itnessbat.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itnesspkfire.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itnesspkfirepillar.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itnesspkflash.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itnesspkflashexplode.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itnesspkthunderball.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itnesspkthundertrail.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itnessyoyo.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itnokonoko.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itoctarock.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itoctarockstone.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itoldkuri.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itoldottosea.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itparasol.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itpatapata.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itpeachexplode.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itpeachparasol.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itpeachtoad.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itpeachtoadspore.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itpeachturnip.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itpikachuthunder.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itpikachutjoltair.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itpikachutjoltground.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itpippi.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itporygon2.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itrabbitc.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itraikou.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itrshell.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itsamusbomb.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itsamuschargeshot.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itsamusgrapple.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itsamusmissile.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itscball.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itseakchain.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itseakneedleheld.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itseakneedlethrown.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itseakvanish.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itsonans.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itspycloak.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itsscope.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itsscopebeam.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itstar.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itstarrod.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itstarrodstar.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itsuikun.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itsword.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ittaru.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ittarucann.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itthunder.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ittincle.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ittogepy.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ittomato.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ittools.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ittosakinto.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itunknown.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itwhispyapple.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itwhitebea.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itwstar.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ityaku.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ityoshiegglay.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ityoshieggthrow.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ityoshistar.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/ityoshitongue.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itzeldadinfire.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itzeldadinfireexplode.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itzgshell.c') },
    @{ Path = (Join-Path $src 'melee/it/kinds/itzrshell.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPurin/ftpurin.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPurin/ftpurinspecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPurin/ftpurinspeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPurin/ftpurinspecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPurin/ftpurinspecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ItemParasolOpen.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ItemParasolFall.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ItemParasolFallSpecial.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ItemParasolDamageFall.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CaptureYoshi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_YoshiEgg.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPeach/ftpeach.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPeach/ftpeachattacks4.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPeach/ftpeachfloat.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPeach/ftpeachfloatattack.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPeach/ftpeachfloatfall.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPeach/ftpeachspecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPeach/ftpeachspeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPeach/ftpeachspecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPeach/ftpeachspecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftYoshi/ftyoshi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftYoshi/ftyoshiguard.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftYoshi/ftyoshispecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftYoshi/ftyoshispeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftYoshi/ftyoshispecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftYoshi/ftyoshispecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftNess/ftness.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftNess/ftnessattackhi4.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftNess/ftnessattacklw4.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftNess/ftnessattacks4.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftNess/ftnessspecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftNess/ftnessspeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftNess/ftnessspecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftNess/ftnessspecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftGameWatch/ftgamewatch.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftGameWatch/ftgamewatchattack100.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftGameWatch/ftgamewatchattack11.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftGameWatch/ftgamewatchattackair.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftGameWatch/ftgamewatchattacklw3.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftGameWatch/ftgamewatchattacks4.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftGameWatch/ftgamewatchspecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftGameWatch/ftgamewatchspeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftGameWatch/ftgamewatchspecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftGameWatch/ftgamewatchspecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_AirCatch.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CaptureMewtwo.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ThrownMewtwo.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftFox/ftfox.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftFox/ftfoxappeals.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftFox/ftfoxspecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftFox/ftfoxspeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftFox/ftfoxspecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftFox/ftfoxspecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftFalco/ftfalco.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPikachu/ftpikachu.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPikachu/ftpikachuspecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPikachu/ftpikachuspeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPikachu/ftpikachuspecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPikachu/ftpikachuspecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftPichu/ftpichu.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftSamus/ftsamus.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftSamus/ftsamusspecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftSamus/ftsamusspeciallw0.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftSamus/ftsamusspeciallw1.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftSamus/ftsamusspecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftSamus/ftsamusspecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftLink/ftlink.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftLink/ftlinkattackair.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftLink/ftlinkspecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftLink/ftlinkspeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftLink/ftlinkspecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftLink/ftlinkspecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCLink/ftclink.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCLink/ftclinkappeals.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftMewtwo/ftmewtwo.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftMewtwo/ftmewtwospecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftMewtwo/ftmewtwospeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftMewtwo/ftmewtwospecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftMewtwo/ftmewtwospecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftZelda/ftzelda.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftZelda/ftzeldaspecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftZelda/ftzeldaspeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftZelda/ftzeldaspecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftZelda/ftzeldaspecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftSeak/ftseak.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftSeak/ftseakspecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftSeak/ftseakspeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftSeak/ftseakspecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftSeak/ftseakspecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_Shouldered.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_09C4.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_0D72.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CaptureCaptain.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CaptureKoopa.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CaptureDamageKoopa.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_CaptureWaitKoopa.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCommon/ftCo_ThrownKoopa.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCaptain/ftcaptain.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCaptain/ftcaptainspecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCaptain/ftcaptainspeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCaptain/ftcaptainspecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftCaptain/ftcaptainspecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftGanon/ftganon.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftMars/ftmars.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftMars/ftmarsspecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftMars/ftmarsspeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftMars/ftmarsspecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftMars/ftmarsspecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftEmblem/ftemblem.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftDonkey/ftdonkey.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftDonkey/ftdonkeyheavyfall.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftDonkey/ftdonkeyheavyjump.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftDonkey/ftdonkeyheavylanding.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftDonkey/ftdonkeyheavyturn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftDonkey/ftdonkeyheavywait0.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftDonkey/ftdonkeyheavywait1.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftDonkey/ftdonkeyheavywalk.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftDonkey/ftdonkeyms3450.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftDonkey/ftdonkeyspecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftDonkey/ftdonkeyspeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftDonkey/ftdonkeyspecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftDonkey/ftdonkeyspecials.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKoopa/ftkoopa.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKoopa/ftkoopaspecialhi.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKoopa/ftkoopaspeciallw.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKoopa/ftkoopaspecialn.c') },
    @{ Path = (Join-Path $src 'melee/ft/kinds/ftKoopa/ftkoopaspecials.c') }
)
$units += New-Slice 'melee/ft/ft_081B.c' 'void ft_80081B38(' @('void ft_80081C88(', 'void ft_80082B1C(', 'void ft_800849EC(', 'Fighter_GObj* ft_80082E3C(', 'void ft_80084DB0(') 'ft_081B_slice'
$units += New-Slice 'melee/ft/ft_0881.c' 'void ft_800881D8(' @('void ft_800881D8(', 'void ft_80088328(', 'void ft_80088478(', 'void ft_80088510(', 'void ft_800885A8(', 'void ft_80088640(', 'void ft_80088770(', 'void ft_800887CC(', 'void ft_80088828(', 'void ft_80088884(', 'void ft_800888E0(', 'void ft_8008893C(', 'void ft_800889F4(', 'static inline int inline0(', 'void ftCo_800886D8(', 'void ft_80088998(', 'void ft_800890BC(', 'static inline void inlineB0(', 'void ft_800890D0(', 'static f32 ft_80089118(', 'f32 ft_80089228(', 'static inline void inlineC0(', 'void ft_800892A0(') 'ft_0881_stale_slice'
# XDK sinf/cosf macros expand to sin/cos; rename locals that shadow them.
$units += New-Adapted 'melee/it/itzako.c' @{ '\bsin\b' = 'zako_sine'; '\bcos\b' = 'zako_cosine' } 'itzako'
# MSVC rejects bitwise OR on pointers; keep the truth test explicit.
$units += New-Adapted 'melee/ef/efasync.c' @{ '(\w+)->ptcl_bank \| \1->tex_bank' = '($1->ptcl_bank || $1->tex_bank)' } 'efasync'
$units += New-Slice 'melee/lb/lb_00B0.c' 'bool lb_8000B074(' @('bool lb_8000B074(', 'void lb_8000C1C0(', 'void lb_8000C228(', 'void lb_8000C290(', 'void lb_8000C2F8(', 'static inline HSD_RObj* robj_next(', 'void lb_8000C390(', 'bool lb_8000B09C(', 'bool lb_8000B134(', 'void lb_8000B804(', 'static void lb_8000B9D8(HSD_JObj* jobj', 'void lb_8000BA0C(', 'static HSD_JObj* lbFindJObjWithAObj(HSD_JObj* jobj)', 'float lbGetJObjCurrFrame(', 'float lbGetJObjEndFrame(', 'static s32 lbGetFreeColorRegImpl(s32 i0, HSD_TevDesc* tevdesc', 's32 lbGetFreeColorRegister(', 's32 lb_8000CC8C(', 's32 lb_8000CCA4(', 's32 lb_8000CD90(', 's32 lb_8000CDA8(', 'void lb_8000CE30(', 'void lb_8000CE40(') 'lb_00B0_constraint_slice'

$objects = @()
$base = @('/nologo','/c','/TC','/O2','/MT','/GS-','/D_XBOX','/DXBOX','/DNDEBUG',
    "/I$matchOverlay", "/I$overlay", "/I$env:XEDK/include/xbox", "/I$(Join-Path $root 'src/xdk')",
    "/I$src", "/I$(Join-Path $src 'sdk_include')",
    "/FI$(Join-Path $root 'src/xdk/gameplay_probe_compat.h')",
    "/FI$overlay/melee/ft/forward.h", "/FI$overlay/melee/ft/kinds/ftCommon/forward.h",
    "/FI$overlay/melee/ft/types.h")
# Quoted kind "forward.h" includes resolve beside the source before /I paths,
# so force every kind overlay (same include guards) ahead of the originals.
$kindForward = @(Get-ChildItem -Path (Join-Path $overlay 'melee/ft/kinds') -Filter 'forward.h' -Recurse |
    Where-Object { $_.Directory.Name -ne 'ftCommon' } | ForEach-Object { "/FI$($_.FullName)" })
foreach ($unit in $units) {
    $dir = if ($unit.Dir) { $unit.Dir } else { Split-Path $unit.Path }
    $object = Join-Path $out ([IO.Path]::GetFileNameWithoutExtension($unit.Path) + '.obj')
    $compilePath = $unit.Path
    $leadInclude = @()
    $unitText = Get-Content -Raw -LiteralPath $unit.Path
    # The XDK C frontend rejects const objects inside other constant initializers.
    $constPattern = '(?m)^static\s+(?:u8|u16|u32|s8|s16|s32|int|MotionFlags)\s+const\s+(\w+)\s*=\s*([^;]+);'
    # It also rounds the FLT_MAX literal 3.4028235e38f above FLT_MAX; use float.h's spelling.
    $fltMaxPattern = '3\.4028235e\+?38f?'
    $adaptedText = [regex]::Replace($unitText, $constPattern, {
        param($m) 'enum { ' + $m.Groups[1].Value + ' = (' + ($m.Groups[2].Value -replace '\s+', ' ').Trim() + ') };'
    })
    $adaptedText = [regex]::Replace($adaptedText, $fltMaxPattern, '3.402823466e+38F')
    if ($adaptedText -match '\b_\s*\[') { $adaptedText = Convert-PaddingDecls $adaptedText }
    if ($adaptedText -cne $unitText) {
        $adaptedDir = Join-Path $out 'const_adapted'
        New-Item -ItemType Directory -Force $adaptedDir | Out-Null
        $compilePath = Join-Path $adaptedDir ([IO.Path]::GetFileName($unit.Path))
        Set-Content -Encoding ASCII -LiteralPath $compilePath $adaptedText
        $leadInclude = @("/I$dir")
    }
    & $Compiler ($leadInclude + $base + $kindForward + @('/W3', "/I$dir", "/Fo$object", $compilePath)) | Write-Host
    if ($LASTEXITCODE -ne 0) { throw "Original fighter unit failed: $($unit.Path)" }
    $objects += $object
}
$nativeBase = @('/nologo','/c','/TC','/O2','/MT','/GS-','/W4','/D_XBOX','/DXBOX','/DNDEBUG',
    "/I$matchOverlay", "/I$overlay", "/I$env:XEDK/include/xbox", "/I$(Join-Path $root 'src/xdk')",
    "/I$src", "/I$(Join-Path $src 'sdk_include')",
    "/FI$(Join-Path $root 'src/xdk/fighter_glue_compat.h')")
foreach ($native in @('fighter_glue_xdk.c', 'fighter_unported_xdk.c', 'particle_draw_xdk.c')) {
    $object = Join-Path $out ([IO.Path]::GetFileNameWithoutExtension($native) + '.obj')
    & $Compiler ($nativeBase + @("/Fo$object", (Join-Path $root "src/xdk/$native"))) | Write-Host
    if ($LASTEXITCODE -ne 0) { throw "Fighter glue failed: $native" }
    $objects += $object
}
return $objects
