[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$DiagnosticDir,
      [Parameter(Mandatory=$true)][string]$OutDir,
      [string]$BuildDir='build-x360/menu-port/full-vs-smoke',
      [switch]$Menus,[string[]]$CaseName=@())
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$scope=(Resolve-Path (Join-Path $root 'build-x360')).Path
$run=(Resolve-Path -LiteralPath $BuildDir).Path
$DiagnosticDir=(Resolve-Path -LiteralPath $DiagnosticDir).Path
New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path
if(-not $run.StartsWith($scope+'\',[StringComparison]::OrdinalIgnoreCase) -or
   -not $OutDir.StartsWith($scope+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Test paths out of scope.' }
if(Test-Path (Join-Path $run 'runtime-trace.txt')) { throw 'Previous run was not archived.' }
if(Get-CimInstance Win32_Process -Filter "Name='xenia_canary.exe'") { throw 'Close the existing Xenia run first.' }
foreach($name in @('default.xex','menus.map','build-info.json')) {
    Copy-Item -LiteralPath (Join-Path $DiagnosticDir $name) -Destination (Join-Path $run $name) -Force
}
$record=Get-Content -Raw (Join-Path $DiagnosticDir 'build-info.json') | ConvertFrom-Json
if([bool]$record.boot_to_match -eq [bool]$Menus) { throw 'Use a menu diagnostic for -Menus and a BootToMatch diagnostic for campaign entries.' }
$cases=@()
if($Menus) {
    $cases=@(
        @{Name='vs';Script='menu_vs_original.txt';Frame=3000;Flow=3;Expect=@('match.select.original: 0','match.select.stage.enter: 0','match.enter.fighters: 2','audio.match.selector_flags: 4')},
        @{Name='vs-music-lr';Script='menu_vs_music_lr.txt';Frame=3000;Flow=3;Expect=@('match.select.original: 0','match.enter.fighters: 2','audio.match.selector_flags: 68','audio.match.background: 1')},
        @{Name='rules-stock';Script='menu_rules_stock.txt';Frame=3600;Flow=3;Expect=@('menu.kind: 13','match.enter.fighters: 2')},
        @{Name='stage-back';Script='menu_stage_back.txt';Frame=2100;Flow=2;Expect=@('match.select.stage.cancel: 1')},
        @{Name='classic';Script='menu_classic_original.txt';Frame=2400;Flow=3;Expect=@('match.select.original: 11','match.classic.ordinal: 0','match.enter.fighters: 2')},
        @{Name='adventure';Script='menu_adventure_original.txt';Frame=2400;Flow=3;Expect=@('match.select.original: 12','match.adventure.scene: 1','match.enter.fighters: 1')},
        @{Name='results';Script='menu_result_return.txt';Frame=4500;Flow=3;Expect=@('match.result.winner: 1','match.select.resume: 0')}
    )
} else {
    $ordinals=@(0,1,3,4,6,7,9)
    for($round=0;$round -lt 7;++$round) {
        $count=if($round -in @(1,3,5)){4}else{2}
        $cases+=@{Name=('classic-{0:D2}' -f $round);Mode=3;Round=$round;Frame=1200;Flow=3;
            Expect=@("match.classic.ordinal: $($ordinals[$round])","match.enter.fighters: $count",'audio.match.background: 1','audio.match.file_found: 1')}
    }
    $scenes=@(1,3,9,10,17,18,25,27,33,35,37,41,43,49,59,65,81,83,89,92)
    $fighters=@(1,3,3,2,1,2,2,1,2,4,2,2,2,4,2,4,3,3,2,2)
    for($round=0;$round -lt 20;++$round) {
        $cases+=@{Name=('adventure-{0:D2}' -f $round);Mode=4;Round=$round;Frame=1200;Flow=3;
            Expect=@('match.mode: 4',"match.adventure.scene: $($scenes[$round])",
                "match.enter.fighters: $($fighters[$round])",'audio.match.background: 1','audio.match.file_found: 1')}
    }
}
if($CaseName.Count) {
    foreach($wanted in $CaseName) {
        if(-not @($cases | Where-Object {$_.Name -eq $wanted}).Count) { throw "Unknown case: $wanted" }
    }
    $cases=@($cases | Where-Object {$CaseName -contains $_.Name})
}
$summary=@()
foreach($case in $cases) {
    $caseDir=Join-Path $OutDir $case.Name
    if(Test-Path -LiteralPath $caseDir) { throw "Existing case: $caseDir" }
    New-Item -ItemType Directory $caseDir | Out-Null
    if($Menus) { $script=Join-Path $root ('tests/fixtures/'+$case.Script) }
    else {
        $script=Join-Path $caseDir 'source-input.txt'
        [IO.File]::WriteAllText($script,"2400 0 0 0 0 -`n",[Text.Encoding]::ASCII)
        $config=@("mode $($case.Mode)","round $($case.Round)",'p1 0','cpu_level 3','seed 12000','human 1') -join "`n"
        [IO.File]::WriteAllText((Join-Path $run 'match-config.txt'),$config+"`n",[Text.Encoding]::ASCII)
    }
    Write-Host "RUN $($case.Name)"
    $failure=$null
    try {
        & (Join-Path $root 'tools/smoke_xenia_menu.ps1') -BuildDir $run -ScriptPath $script `
            -Expect $case.Expect -TargetFrame $case.Frame -ExpectedFlow $case.Flow -MinHits 0 -Seconds 100
    } catch { $failure=$_ }
    foreach($name in @('runtime-trace.txt','xenia.log','result.json','input-script.txt','match-config.txt','owned-pid.txt')) {
        $path=Join-Path $run $name
        if(Test-Path -LiteralPath $path) { Move-Item -LiteralPath $path -Destination (Join-Path $caseDir $name) }
    }
    $evidence=@{DiagnosticDir=$DiagnosticDir;XexSha256=$record.xex_sha256;MapSha256=$record.map_sha256}
    $evidence | ConvertTo-Json | Set-Content (Join-Path $caseDir 'binary-evidence.json') -Encoding UTF8
    if(Test-Path (Join-Path $caseDir 'result.json')) {
        $result=Get-Content -Raw (Join-Path $caseDir 'result.json') | ConvertFrom-Json
        $summary+=[pscustomobject]@{Case=$case.Name;Pass=$result.Pass;Frame=$result.LoopFrame;Hits=$result.Hits;Missing=$result.MissingMarkers}
        $summary | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $OutDir 'summary.json') -Encoding UTF8
    }
    if($failure) { throw $failure }
}
$summary | Format-Table
