[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$DiagnosticDir,
      [Parameter(Mandatory=$true)][string]$OutDir,
      [string]$BuildDir='build-x360/menu-port/full-vs-smoke')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$scope=(Resolve-Path (Join-Path $root 'build-x360')).Path
$run=(Resolve-Path -LiteralPath $BuildDir).Path
$DiagnosticDir=(Resolve-Path -LiteralPath $DiagnosticDir).Path
$out=[IO.Path]::GetFullPath($OutDir)
foreach($path in @($run,$out,$DiagnosticDir)) {
    if(-not $path.StartsWith($scope+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Test path outside build-x360.' }
}
if(Test-Path -LiteralPath $out) { throw 'Use a fresh output folder.' }
if(Test-Path (Join-Path $run 'runtime-trace.txt')) { throw 'Previous run was not archived.' }
if(Get-CimInstance Win32_Process -Filter "Name='xenia_canary.exe'") { throw 'Close the existing Xenia run first.' }
$record=Get-Content -Raw (Join-Path $DiagnosticDir 'build-info.json') | ConvertFrom-Json
if(-not $record.boot_to_match -or $record.normal.input_script -or $record.normal.boot_to_match) {
    throw 'Expected a separate BootToMatch diagnostic based on a normal build.'
}
foreach($entry in @(@('default.xex','xex_sha256'),@('menus.map','map_sha256'))) {
    if((Get-FileHash (Join-Path $DiagnosticDir $entry[0])).Hash -ne $record.($entry[1])) { throw 'Diagnostic hash mismatch.' }
}
New-Item -ItemType Directory $out | Out-Null
foreach($name in @('default.xex','menus.map','build-info.json')) {
    Copy-Item -LiteralPath (Join-Path $DiagnosticDir $name) -Destination (Join-Path $run $name) -Force
}
# Derive native roster -> original FighterKind IDs; they are not CharacterKind IDs.
$roster=[regex]::Matches((Get-Content -Raw (Join-Path $root 'src/xdk/fighter_glue_xdk.c')),'(?m)^    \{ (Ft_Kind_\w+),')
$kinds=@{}
foreach($m in [regex]::Matches((Get-Content -Raw (Join-Path $root 'upstream/melee-pc/src/melee/ft/forward.h')),'/\* ([0-9A-F]{2}) \*/ (Ft_Kind_\w+),')) {
    $kinds[$m.Groups[2].Value]=[Convert]::ToInt32($m.Groups[1].Value,16)
}
if($roster.Count -ne 30) { throw 'Review native roster coverage after changing its size.' }
$cases=@()
for($first=0;$first -lt 26;$first+=4) {
    $indices=@(0..3 | ForEach-Object {($first+$_)%26})
    $expected=@('match.enter.fighters: 4','fighter.collision.init.top: 8','audio.match.background: 1')
    foreach($index in $indices) {
        $name=$roster[$index].Groups[1].Value
        if(-not $kinds.ContainsKey($name)) { throw "Missing original kind: $name" }
        $expected+="fighter.collision.init.kind: $($kinds[$name])"
    }
    if($indices -contains 25) { $expected+="fighter.collision.init.kind: $($kinds['Ft_Kind_Nana'])" }
    $cases+=@{Name=('roster-{0:D2}' -f $first);Frame=600;Expect=$expected;Script="2400 0 0 0 0 -`n";
        Config=@('mode 2',"stage $(($first/4)%5)",'players 4','human 0','cpu_level 3','stocks 4','items 3','seed 12000',
                 "p1 $($indices[0])","p2 $($indices[1])","p3 $($indices[2])","p4 $($indices[3])",'c1 1','c2 2','c3 3','c4 4')}
}
foreach($index in @(13,14)) {
    $cases+=@{Name="transform-$index";Frame=1800;Expect=@('fighter.transform.player: 0','fighter.collision.init.top: 8');
        Transform=$true;Script="90 0 0 0 0 -`n3 0 -127 0 0 B`n180 0 0 0 0 -`nloop`n";
        Config=@('mode 2','stage 1','players 2','human 1','cpu_level 1','stocks 8','items -1',"p1 $index",'p2 0','seed 12000')}
}
$cases+=@{Name='life-reset';Frame=1800;Expect=@('fighter.life_reset.done: 0','fighter.collision.init.top: 8');
    LifeReset=$true;Script="250 0 0 0 0 -`n240 127 0 0 0 -`n90 0 -127 0 0 -`n60 0 0 0 0 -`nloop`n";
    Config=@('mode 2','stage 1','players 2','human 1','cpu_level 1','stocks 8','items -1','p1 9','p2 0','seed 12000')}
$summary=@()
foreach($case in $cases) {
    $caseDir=Join-Path $out $case.Name
    New-Item -ItemType Directory $caseDir | Out-Null
    $script=Join-Path $caseDir 'source-input.txt'
    [IO.File]::WriteAllText($script,$case.Script,[Text.Encoding]::ASCII)
    [IO.File]::WriteAllText((Join-Path $run 'match-config.txt'),($case.Config -join "`n")+"`n",[Text.Encoding]::ASCII)
    Write-Host "RUN $($case.Name)"
    $failure=$null
    try {
        & (Join-Path $root 'tools/smoke_xenia_menu.ps1') -BuildDir $run -ScriptPath $script -Expect $case.Expect -TargetFrame $case.Frame -MinHits 0 -Seconds 100
        $trace=Get-Content -Raw (Join-Path $run 'runtime-trace.txt')
        if($trace -match 'fighter.collision.bones_failed|match.enter.spawn_failed') { throw 'Native collision binding/spawn failure.' }
        if($case.Transform -and [regex]::Matches($trace,'(?m)^fighter.transform.player: 0\r?$').Count -lt 2) { throw 'Fewer than two transformations.' }
        if($case.LifeReset -and [regex]::Matches($trace,'(?m)^fighter.collision.init.kind: 1\r?$').Count -lt 2) { throw 'Fox collision did not initialize again after losing a life.' }
    } catch { $failure=$_ }
    foreach($name in @('runtime-trace.txt','xenia.log','result.json','input-script.txt','match-config.txt','owned-pid.txt')) {
        $path=Join-Path $run $name
        if(Test-Path -LiteralPath $path) { Move-Item -LiteralPath $path -Destination (Join-Path $caseDir $name) }
    }
    @{DiagnosticDir=$DiagnosticDir;XexSha256=$record.xex_sha256;MapSha256=$record.map_sha256} | ConvertTo-Json | Set-Content (Join-Path $caseDir 'binary-evidence.json') -Encoding UTF8
    if(-not (Test-Path (Join-Path $caseDir 'result.json'))) {
        if($failure) { throw $failure }
        throw 'Missing native smoke result.'
    }
    $result=Get-Content -Raw (Join-Path $caseDir 'result.json') | ConvertFrom-Json
    $result | Add-Member -NotePropertyName CollisionChecks -NotePropertyValue (-not [bool]$failure)
    $result.Pass=$result.Pass -and -not [bool]$failure
    $result | ConvertTo-Json | Set-Content (Join-Path $caseDir 'result.json') -Encoding UTF8
    $summary+=[pscustomobject]@{Case=$case.Name;Pass=($result.Pass -and -not $failure);Frame=$result.LoopFrame;Hits=$result.Hits}
    $summary | ConvertTo-Json | Set-Content (Join-Path $out 'summary.json') -Encoding UTF8
    if($failure) { throw $failure }
}
$summary | Format-Table
