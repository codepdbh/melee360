[CmdletBinding()]
param([Parameter(Mandatory)][string] $OutputPath)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$classic = Get-Content -Raw (Join-Path $root 'upstream/melee-pc/src/melee/gm/gmclassic.c')
$start = $classic.IndexOf('static gmClassic_803DDEC8Data gmClassic_803DDEC8 =')
if ($start -lt 0) { throw 'Original Classic table not found.' }
$cursor = $classic.IndexOf('{', $start) + 1
$depth = 1
$groups = @()
$groupStart = 0
while ($depth -gt 0 -and $cursor -lt $classic.Length) {
    if ($classic[$cursor] -eq '{') {
        if ($depth -eq 1) { $groupStart = $cursor }
        ++$depth
    }
    if ($classic[$cursor] -eq '}') {
        --$depth
        if ($depth -eq 1) { $groups += $classic.Substring($groupStart, $cursor - $groupStart + 1) }
    }
    ++$cursor
}
if ($groups.Count -lt 3) { throw 'Original Classic table layout changed.' }
$normal = [regex]::Matches($groups[2], '\{\s*(\d+),\s*\{\s*(\d+),\s*33,\s*33\s*\},\s*0\s*\}')
if ($normal.Count -ne 38) { throw "Expected 38 normal Classic matchups; got $($normal.Count)." }
$round = [regex]::Match($groups[0], '\{\s*0x00,\s*0x00,\s*0,\s*0,\s*(\d+),')
if (-not $round.Success) { throw 'Normal Classic time limit not found.' }
$stageSource = Get-Content -Raw (Join-Path $root 'upstream/melee-pc/src/melee/gr/stage.c')
$stageTable = [regex]::Match($stageSource, '(?s)struct StageIdMapEntry stage_id_map\[\] = \{(.*?)\n\};')
$stageMap = [regex]::Matches($stageTable.Groups[1].Value, '\{\s*(Gr_Kind_\w+),\s*0,\s*0\s*\}')
$playerSource = Get-Content -Raw (Join-Path $root 'upstream/melee-pc/src/melee/pl/player.c')
$playerTable = [regex]::Match($playerSource, '(?s)ftMapping ftMapping_list\[ChKind_Max\] = \{(.*?)\n\};')
$characterMap = [regex]::Matches($playerTable.Groups[1].Value, '/\*\s*(?:CKind|ChKind)_\w+\s*\*/\s*\{\s*(Ft_Kind_\w+)')
$grValues = @{}
$ftValues = @{}
foreach ($entry in [regex]::Matches((Get-Content -Raw (Join-Path $root 'upstream/melee-pc/src/melee/gr/forward.h')), '/\*\s*0x([0-9A-Fa-f]+)\s*\*/\s*(Gr_Kind_\w+)')) {
    $grValues[$entry.Groups[2].Value] = [Convert]::ToInt32($entry.Groups[1].Value, 16)
}
foreach ($entry in [regex]::Matches((Get-Content -Raw (Join-Path $root 'upstream/melee-pc/src/melee/ft/forward.h')), '/\*\s*([0-9A-Fa-f]{2})\s*\*/\s*(Ft_Kind_\w+)')) {
    $ftValues[$entry.Groups[2].Value] = [Convert]::ToInt32($entry.Groups[1].Value, 16)
}
$lines = @('// Generated from original gmclassic.c, stage.c and player.c; do not edit.',
    '#ifndef MELEE360_CLASSIC_MATCHUPS_H', '#define MELEE360_CLASSIC_MATCHUPS_H',
    'typedef struct M360ClassicMatchup { unsigned stkind; int grkind; int fighterKind; } M360ClassicMatchup;',
    "enum { kClassicNormalSeconds = $($round.Groups[1].Value) };",
    'static const M360ClassicMatchup g_m360ClassicNormal[] = {')
foreach ($entry in $normal) {
    $stage = [int]$entry.Groups[1].Value
    $character = [int]$entry.Groups[2].Value
    if ($stage -ge $stageMap.Count -or $character -ge $characterMap.Count) { throw 'Classic mapping index out of bounds.' }
    $grName = $stageMap[$stage].Groups[1].Value
    $ftName = $characterMap[$character].Groups[1].Value
    if (-not $grValues.ContainsKey($grName) -or -not $ftValues.ContainsKey($ftName)) { throw 'Unmapped Classic ground/fighter.' }
    $lines += "    { $stage, $($grValues[$grName]), $($ftValues[$ftName]) }, // $grName / $ftName"
}
$lines += @('};', '#endif')
Set-Content -LiteralPath $OutputPath -Encoding ASCII $lines
