[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$source = Join-Path $PSScriptRoot 'soak_xenia.ps1'
$tokens = $null
$parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($source, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw 'Soak harness parse failed.' }
foreach ($name in @('Read-Shared', 'Read-Trace', 'Get-Verdict', 'Test-RunComplete', 'New-Config', 'Get-SoakMatrix', 'Resolve-Address')) {
    $function = $ast.Find({ param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq $name }, $true)
    if (-not $function) { throw "Missing harness function: $name" }
    Invoke-Expression $function.Extent.Text
}
$out = Join-Path $root 'build-x360/host-tests/soak-rules'
New-Item -ItemType Directory -Force $out | Out-Null
$fixture = Join-Path $out 'runtime-trace.txt'
function Read-Fixture([string] $text) {
    [IO.File]::WriteAllText($fixture, $text)
    Read-Trace $fixture
}
function Assert-Rule([bool] $condition, [string] $message) {
    if (-not $condition) { throw $message }
}
$soakMap = Join-Path $out 'resolver.map'
[IO.File]::WriteAllText($soakMap, " 0003:00000000 00000100H .text CODE`n 0003:00000000 First 82060000 f fixture.obj`n 0003:00000040 Next 82060040 f fixture.obj`n")
$script:MapSymbols = $null; $script:MapCodeEnd = 0
Assert-Rule ((Resolve-Address '0x82060048') -eq 'Next+0x8') 'Valid code address did not resolve.'
Assert-Rule ((Resolve-Address '0x82060120') -eq '0x82060120') 'Heap/data pointer was mislabeled as the last function.'
Assert-Rule ((Resolve-Address '0x8205FFFF') -eq '0x8205FFFF') 'Address before code was mislabeled.'
$config = [pscustomobject]@{ Script = 'adventurepilot'; Repeat = 1; TargetFrame = 99999; Mode = 4; Seed = -1 }
$trace = Read-Fixture "match.campaign.preview_complete: 4`n"
Assert-Rule (-not $trace.CampaignComplete) 'Final match marker must not establish flow completion.'
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'SHORT') 'Incomplete flow incorrectly passed.'
$trace = Read-Fixture "mode.preview_complete: 4`nloop.flow_state: 2`n"
Assert-Rule $trace.CampaignComplete 'Flow completion marker was missed.'
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'OK') 'Completed flow failed.'
$config.Seed = 12000
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'SHORT') 'An ignored diagnostic seed must not pass.'
Assert-Rule (-not (Test-RunComplete $config "mode.preview_complete: 4`n" $false)) 'Seed mismatch stopped the run.'
$trace = Read-Fixture "auto.random.seed: 12000`nmode.preview_complete: 4`n"
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'OK') 'Matching diagnostic seed was rejected.'
Assert-Rule (Test-RunComplete $config "auto.random.seed: 12000`nmode.preview_complete: 4`n" $false) 'Matching seeded completion did not stop the run.'
$config.Seed = -1
$trace = Read-Fixture "mode.preview_complete: 3`n"
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'SHORT') 'Classic completion must not establish an Adventure clear.'
Assert-Rule (-not (Test-RunComplete $config "mode.preview_complete: 3`n" $false)) 'Wrong campaign mode stopped the Adventure run.'
$trace = Read-Fixture "loop.match_frame: 5100`nloop.match_hits: 26`nloop.match_frame: 0`nloop.match_hits: 0`n"
Assert-Rule ($trace.MatchFrame -eq 5100 -and $trace.Hits -eq 26) 'Next-match reset lost campaign evidence.'
$trace = Read-Fixture "match.adventure.scene: 92`nmatch.load.failed: 1`n"
Assert-Rule (-not $trace.GigaEntered) 'Scene selection alone must not prove fighter load.'
$trace = Read-Fixture "match.adventure.scene: 92`nmatch.enter.fighters: 2`n"
Assert-Rule $trace.GigaEntered 'Successful Giga entry was missed.'
$trace = Read-Fixture "match.adventure.scene: 92`nmatch.adventure.scene: 89`nmatch.enter.fighters: 2`n"
Assert-Rule (-not $trace.GigaEntered) 'Other scene load incorrectly established Giga entry.'
$config.Script = 'wavepilot'
$trace = Read-Fixture "match.result.winner: 1`n"
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'SHORT') 'Losing a wave incorrectly passed.'
$trace = Read-Fixture "match.result.winner: 0`nmatch.result.winner: 1`n"
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'OK') 'Prior wave victory was lost.'
$trace = Read-Fixture "mode.preview_complete: 4`nhang.frame: 123`n"
$config.Script = 'adventurepilot'
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'FREEZE') 'Hang must take precedence over completion.'
$trace = Read-Fixture "hang.frame: 123`nhang.detail: 4098`nloop.frame: 900`n"
Assert-Rule ($trace.HangDetail -eq '0x1002') 'Renderer operation detail was not preserved.'
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'FREEZE') 'Progress after a watchdog must not silently pass.'
$config.Script = 'mazepilot'
Assert-Rule (-not (Test-RunComplete $config "auto.match.done: 1`n" $false)) 'Maze must keep running after an ordinary match-end marker.'
Assert-Rule (Test-RunComplete $config "match.adventure.maze_goal: 123`n" $false) 'Maze goal must stop the run.'
$config.Script = 'gigapilot'
Assert-Rule (-not (Test-RunComplete $config "match.adventure.scene: 92`nmatch.adventure.scene: 89`nmatch.enter.fighters: 2`n" $false)) 'Polling mixed another scene with Giga entry.'
Assert-Rule (Test-RunComplete $config "match.adventure.scene: 92`nmatch.enter.fighters: 2`n" $false) 'Polling missed valid Giga entry.'
$config.Script = 'mazepilot'
$trace = Read-Fixture "match.adventure.maze.triforce: 0`n"
Assert-Rule (-not $trace.MazeGoal) 'Touching the Triforce must not establish completed results.'
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'SHORT') 'Maze transition incorrectly passed before completion.'
$trace.MatchesDone = 1
$trace.MatchFrame = 100000
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'SHORT') 'Ended match or frame target must not prove a maze clear.'
Assert-Rule ((Get-Verdict $config $trace $true $false) -eq 'CRASH') 'Incomplete maze exit must be reported.'
foreach ($pilot in @('routepilot', 'escapepilot', 'adventurepilot', 'wavepilot', 'gigapilot')) {
    $config.Script = $pilot
    Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'SHORT') "$pilot incorrectly passed without its objective marker."
}
$config.Script = 'mazepilot'
$trace = Read-Fixture "match.adventure.maze_goal: 123`n"
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'OK') 'Completed maze was missed.'
$trace = Read-Fixture "match.adventure.maze_goal: 123`nhang.frame: 124`n"
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'FREEZE') 'Maze completion must not mask a hang.'
$config.Script = 'racepilot'
$trace = Read-Fixture "match.adventure.race.checkpoint: 3`nauto.match.done: 1`n"
Assert-Rule (-not $trace.RaceGoal) 'Race checkpoint must not establish completion.'
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'SHORT') 'Race timeout incorrectly passed.'
Assert-Rule (-not (Test-RunComplete $config "auto.match.done: 1`n" $false)) 'Race must continue until its goal.'
$trace = Read-Fixture "match.adventure.race_goal: 123`n"
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'OK') 'Race completion was missed.'
Assert-Rule (Test-RunComplete $config "match.adventure.race_goal: 123`n" $false) 'Race goal must stop the run.'
$Fighters = @('Mario', 'Link'); $Stages = @('Battlefield', 'Temple')
$AdventurePhaseCount = 20
$matrix20 = @(Get-SoakMatrix)
Assert-Rule (($matrix20 | Where-Object Name -eq 'adventure-from-maze-pilot').Round -eq 4) 'Maze transition campaign must start at the maze.'
Assert-Rule (($matrix20 | Where-Object Name -eq 'adventure-from-maze-pilot').Script -eq 'adventurepilot') 'Maze transition campaign must require campaign completion, not just a maze clear.'
Assert-Rule (($matrix20 | Where-Object Name -eq 'adventure-from-maze-very-easy-pilot').CpuLevel -eq 1) 'Very Easy transition test must use the original lowest difficulty.'
Assert-Rule (($matrix20 | Where-Object Name -eq 'adventure-wireframe-wave-pilot').Round -eq 16) 'Twenty-phase wireframe round shifted.'
Assert-Rule (-not ($matrix20 | Where-Object Name -eq 'adventure-race-pilot')) 'Twenty-phase build must not select the race.'
$AdventurePhaseCount = 21
$matrix21 = @(Get-SoakMatrix)
Assert-Rule (($matrix21 | Where-Object Name -eq 'adventure-wireframe-wave-pilot').Round -eq 17) 'Twenty-one-phase wireframe round did not shift.'
Assert-Rule (($matrix21 | Where-Object Name -eq 'adventure-bowser-easy-final-pilot').Round -eq 19) 'Twenty-one-phase final round did not shift.'
Assert-Rule (($matrix21 | Where-Object Name -eq 'adventure-race-pilot').Round -eq 14) 'Race pilot selects the wrong round.'
foreach ($phaseCount in @(20, 21)) {
    $header = Join-Path $out "adventure-$phaseCount.h"
    $generatorArgs = @((Join-Path $PSScriptRoot 'generate_adventure_matchups.py'), $header)
    if ($phaseCount -eq 21) { $generatorArgs += '--experimental-race' }
    & python $generatorArgs
    if ($LASTEXITCODE -ne 0) { throw 'Adventure metadata generation failed.' }
    $generated = Get-Content -Raw $header
    $encounters = [regex]::Match($generated, '(?s)g_m360Adventure\[\] = \{(.*?)\n\};').Groups[1].Value
    $scenes = @([regex]::Matches($encounters, '(?m)^\s*\{\s*(\d+),') | ForEach-Object { [int]$_.Groups[1].Value })
    Assert-Rule ($scenes.Count -eq $phaseCount) 'Generated Adventure phase count differs from the build option.'
    Assert-Rule (($scenes -contains 58) -eq ($phaseCount -eq 21)) 'Experimental race leaked into the default build or is absent from the prototype.'
}
Write-Host '[M360][HOST] Soak evidence and phase matrix rules PASS'
