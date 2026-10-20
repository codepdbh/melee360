[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$DiagnosticDir,
      [Parameter(Mandatory=$true)][string]$OutDir,
      [string]$BuildDir='build-x360/menu-port/full-vs-smoke',
      [int[]]$Stages=@(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16),
      [ValidateRange(600,10000)][int]$Frames=1200)
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
foreach($stage in $Stages) { if($stage -lt 0 -or $stage -gt 16) { throw 'Select a supported VS stage index (0-16).' } }
New-Item -ItemType Directory $out | Out-Null
foreach($name in @('default.xex','menus.map','build-info.json')) {
    Copy-Item -LiteralPath (Join-Path $DiagnosticDir $name) -Destination (Join-Path $run $name) -Force
}
Copy-Item -LiteralPath (Join-Path $DiagnosticDir 'build-info.json') -Destination (Join-Path $out 'binary-evidence.json')
$cases=@($Stages | ForEach-Object { @{Name=('stage-{0:D2}' -f $_);Stage=$_;Frequency=4} })
$cases+=@{Name='items-off';Stage=0;Frequency=-1}
$summary=@()
foreach($case in $cases) {
    $caseDir=Join-Path $out $case.Name
    New-Item -ItemType Directory $caseDir | Out-Null
    $script=Join-Path $caseDir 'source-input.txt'
    "$(($Frames+600)) 0 0 0 0 -" | Set-Content -Encoding ASCII $script
    @('mode 2',"stage $($case.Stage)",'players 2','human 0','cpu_level 3','stocks 99',
      "items $($case.Frequency)",'seed 12000','p1 0','p2 9') |
        Set-Content -Encoding ASCII (Join-Path $run 'match-config.txt')
    $failure=$null
    $attempts=0
    $spawns=@()
    try {
        $disabled=switch($case.Stage) { 9 {12}; 10 {2}; 13 {6}; 14 {6}; default {0} }
        $expected=@('match.camera.output_width: 1280','match.enter.fighters: 2','audio.match.background: 1',"match.stage.initial_disabled: $disabled")
        if($case.Stage -eq 13) { $expected+=@('match.stage.points.ready: 1','match.stage.point_count: 20') }
        & (Join-Path $root 'tools/smoke_xenia_menu.ps1') -BuildDir $run -ScriptPath $script `
            -Expect $expected `
            -TargetFrame $Frames -MinHits 0 -Seconds 100
        $trace=Get-Content -Raw (Join-Path $run 'runtime-trace.txt')
        $attempts=[regex]::Matches($trace,'(?m)^item.random.attempt: 1\r?$').Count
        $spawns=[regex]::Matches($trace,'(?m)^item.random.spawn.kind: (\d+)\r?$')
        if($case.Frequency -ge 0 -and $spawns.Count -lt 1) { throw 'No real random item was created.' }
        if($case.Frequency -lt 0 -and ($attempts -or $spawns.Count)) { throw 'Disabled random items still spawned.' }
    } catch { $failure=$_ }
    foreach($name in @('runtime-trace.txt','xenia.log','result.json','input-script.txt','match-config.txt','owned-pid.txt')) {
        $path=Join-Path $run $name
        if(Test-Path -LiteralPath $path) { Move-Item -LiteralPath $path -Destination (Join-Path $caseDir $name) }
    }
    $resultPath=Join-Path $caseDir 'result.json'
    if(-not (Test-Path $resultPath)) { if($failure){throw $failure};throw 'Missing native result.' }
    $result=Get-Content -Raw $resultPath | ConvertFrom-Json
    $result | Add-Member -NotePropertyName ItemChecks -NotePropertyValue (-not [bool]$failure)
    $result | Add-Member -NotePropertyName RandomSpawns -NotePropertyValue $spawns.Count
    $result | Add-Member -NotePropertyName SpawnKinds -NotePropertyValue @($spawns | ForEach-Object {[int]$_.Groups[1].Value} | Sort-Object -Unique)
    $result.Pass=$result.Pass -and -not [bool]$failure
    $result | ConvertTo-Json -Depth 5 | Set-Content -Encoding UTF8 $resultPath
    $summary+=[pscustomobject]@{Case=$case.Name;Pass=$result.Pass;Frame=$result.LoopFrame;Spawns=$spawns.Count;Hits=$result.Hits}
    $summary | ConvertTo-Json | Set-Content -Encoding UTF8 (Join-Path $out 'summary.json')
    if($failure) { throw $failure }
}
$summary | Format-Table
