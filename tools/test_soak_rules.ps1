[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$source = Join-Path $PSScriptRoot 'soak_xenia.ps1'
$tokens = $null
$parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($source, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw 'Soak harness parse failed.' }
foreach ($name in @('Read-Shared', 'Read-Trace', 'Get-Verdict')) {
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
$config = [pscustomobject]@{ Script = 'adventurepilot'; Repeat = 1; TargetFrame = 99999 }
$trace = Read-Fixture "match.campaign.preview_complete: 4`n"
Assert-Rule (-not $trace.CampaignComplete) 'Final match marker must not establish flow completion.'
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'SHORT') 'Incomplete flow incorrectly passed.'
$trace = Read-Fixture "mode.preview_complete: 4`nloop.flow_state: 2`n"
Assert-Rule $trace.CampaignComplete 'Flow completion marker was missed.'
Assert-Rule ((Get-Verdict $config $trace $false $false) -eq 'OK') 'Completed flow failed.'
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
Write-Host '[M360][HOST] Soak evidence rules PASS'
