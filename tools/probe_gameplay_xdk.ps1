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
$sources = @('melee/ft/fighter.c', 'melee/ft/kinds/ftCommon/ftCo_Wait.c', 'melee/gm/gmtitle.c', 'melee/mn/mn_22EC.c',
    'melee/ft/ftcommon.c', 'melee/ft/ftanim.c', 'melee/ft/ftaction.c',
    'melee/ft/ftcoll.c', 'melee/ft/ftparts.c', 'melee/ft/ftdata.c',
    'melee/ft/kinds/ftCommon/ftCo_Walk.c',
    'melee/ft/kinds/ftCommon/ftCo_Jump.c',
    'melee/ft/kinds/ftCommon/ftCo_Fall.c',
    'melee/ft/kinds/ftCommon/ftCo_Dash.c',
    'melee/ft/kinds/ftCommon/ftCo_Run.c',
    'melee/ft/kinds/ftCommon/ftCo_Turn.c',
    'melee/ft/kinds/ftCommon/ftCo_Landing.c',
    'melee/ft/kinds/ftCommon/ftCo_Attack1.c',
    'melee/ft/kinds/ftCommon/ftCo_AttackAir.c')
$failed = 0
foreach ($source in $sources) {
    $name = [IO.Path]::GetFileNameWithoutExtension($source)
    $inputSource = "$root/upstream/melee-pc/src/$source"
    if ($name -in @('ftanim','ftparts','ftdata')) {
        $adapted = Get-Content -Raw $inputSource
        if ($name -eq 'ftanim') {
            $adapted = $adapted.Replace('    r5->n_costume_tobjs = r4->x8;', "    DiscU32* xC;`r`n    r5->n_costume_tobjs = r4->x8;")
            $adapted = $adapted.Replace('    DiscU32* xC = DP(DiscU32, r4->xC);', '    xC = DP(DiscU32, r4->xC);')
            # Native 32-bit DP is an identity macro; these tables are read as
            # raw four-byte slots, not as their declared pointee structures.
            $adapted = [regex]::Replace($adapted, 'DP\(DiscU32, ([\w>\-\.]+)\)', '((DiscU32*)($1))')
            $adapted = $adapted.Replace('DP(DiscU16, xC[fp->x619_costume_id].v)', '((DiscU16*)(uintptr_t)xC[fp->x619_costume_id].v)')
            $adapted = $adapted.Replace('DP(DiscU16, xC[0].v)', '((DiscU16*)(uintptr_t)xC[0].v)')
            $adapted = $adapted.Replace('    FighterBone* bone = ftParts_GetBone(fp, 0x35);', "    float rotation;`r`n    FighterBone* bone = ftParts_GetBone(fp, 0x35);")
            $adapted = $adapted.Replace('    float rotation = ftPartGetRotX(fp, ftParts_GetBoneIndex(fp, 0x35));', '    rotation = ftPartGetRotX(fp, ftParts_GetBoneIndex(fp, 0x35));')
        }
        if ($name -eq 'ftparts') {
            $adapted = [regex]::Replace($adapted, '(\b(?:void|f32) ftPart(?:Set|Get)Rot[XYZ]\([^)]*\)\s*\{)', '$1' + "`r`n    HSD_JObj* jobj;")
            $adapted = [regex]::Replace($adapted, '    HSD_JObj\* jobj = ((?:fp|arg0)->parts\[\(u8\) part_idx\]\.joint);', '    jobj = $1;')
        }
        if ($name -eq 'ftdata') {
            $adapted = $adapted.Replace('        struct Fighter_WaitAnimData* anims = DP(struct Fighter_WaitAnimData, temp_r27->xC);', '        anims = DP(struct Fighter_WaitAnimData, temp_r27->xC);')
            $adapted = $adapted.Replace('    if (ftData_Table_Unk0[kind].data == NULL) {', "    if (ftData_Table_Unk0[kind].data == NULL) {`r`n        struct Fighter_WaitAnimData* anims;")
        }
        $inputSource = "$output/$name.c"
        Set-Content -Encoding ASCII $inputSource $adapted
    }
    $language = '/TC'
    $arguments = @('/nologo','/c',$language,'/O2','/D_XBOX','/DXBOX','/DNDEBUG',
        "/I$overlay", "/I$env:XEDK/include/xbox", "/I$root/src/xdk",
        "/I$root/upstream/melee-pc/src", "/I$root/upstream/melee-pc/src/sdk_include",
        "/FI$root/src/xdk/gameplay_probe_compat.h", "/Fo$output/$name.obj",
        "/I$(Split-Path "$root/upstream/melee-pc/src/$source")", $inputSource)
    # Force overlay guards before source-relative includes can select originals.
    $forced = @("/FI$overlay/melee/ft/forward.h", "/FI$overlay/melee/ft/kinds/ftCommon/forward.h")
    $lines = & $compiler @arguments @forced
    $result = $LASTEXITCODE
    $lines | Out-File -Encoding utf8 "$output/$name.log"
    Write-Output "$source exit=$result"
    $lines | Select-String 'error ' | Select-Object -First 8
    if ($result -ne 0) { ++$failed }
}
if ($failed) { throw "$failed gameplay compilation probes failed; see $output" }
