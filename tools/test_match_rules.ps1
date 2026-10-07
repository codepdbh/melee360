[CmdletBinding()]
param([string] $VcVars = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $VcVars) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $install = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $install) { throw 'MSVC host compiler not found; specify -VcVars.' }
    $VcVars = Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
}
$out = Join-Path $root 'build-x360/host-tests/match-rules'
New-Item -ItemType Directory -Force $out | Out-Null
& (Join-Path $PSScriptRoot 'generate_classic_matchups.ps1') -OutputPath (Join-Path $out 'melee_classic_matchups.h')
& python (Join-Path $PSScriptRoot 'generate_adventure_matchups.py') (Join-Path $out 'melee_adventure_matchups.h') --experimental-race
if ($LASTEXITCODE -ne 0) { throw 'Adventure generation failed.' }
$source = (Get-Content -Raw (Join-Path $root 'src/xdk/match_scene_xdk.c')).Replace("`r`n", "`n")
function Get-MatchFunction([string] $Signature) {
    $start = $source.IndexOf($Signature)
    if ($start -lt 0) { throw "Missing function: $Signature" }
    $cursor = $source.IndexOf('{', $start) + 1
    $depth = 1
    while ($depth -gt 0 -and $cursor -lt $source.Length) {
        if ($source[$cursor] -eq '{') { ++$depth }
        if ($source[$cursor] -eq '}') { --$depth }
        ++$cursor
    }
    if ($depth) { throw "Unbalanced function: $Signature" }
    $source.Substring($start, $cursor - $start)
}
$extracted = ''
foreach ($signature in @(('static int IsCampaign(void)' + "`n{"), 'static unsigned TimeMinutes(void)',
    'unsigned M360_MatchCampaignRounds(', 'static const M360AdventureEncounter* AdventureEncounter(',
    'unsigned M360_MatchNextCampaignRound(',
    'static unsigned CampaignSeconds(', 'int M360_MatchIsTeams(', 'int M360_MatchTeam(',
    'float M360_MatchCombatRatio(',
    'unsigned M360_MatchCpuLevel(',
    'unsigned M360_MatchCpuKind(',
    'static StageParam* MatchStageParam(', 'int M360_MatchBgmId(', 'int M360_MatchGroundKind(',
    'static unsigned StartingStocks(', 'static int LoseStock(', 'static int StockResult(',
    'static int ClassicStageIndex(', 'static void CampaignOpponent(', 'static void ResolveCostumes(', ('static void StartFight(void)' + "`n{"),
    'void M360_MatchEnter(', 'void M360_MatchLeave(',
    'static int SelectInput(', 'static unsigned P1Slot(', ('static int SelectFrame(void)' + "`n{"),
    'static int OutsideBlastZone(', 'static void SetCollisionGroupEnabled(', 'static void DisableCollisionGroup(',
    'static void MazeRoomBounds(', 'unsigned M360_MatchMazeVisited(', 'int M360_MatchMazePoint(', 'static void MazeFrame(', 'static int CollisionGroupHasFloor(', 'static void AdventureFrame(', 'void M360_MatchSetMode(', 'int M360_MatchFrame(')) {
    $extracted += (Get-MatchFunction $signature) + "`r`n"
}
$source = (Get-Content -Raw (Join-Path $root 'src/xdk/fighter_glue_xdk.c')).Replace("`r`n", "`n")
$extracted = (Get-MatchFunction 'unsigned M360_FighterCostumeCount(') + "`r`n" +
    (Get-MatchFunction 'int M360_FighterIndexForKind(') + "`r`n" + $extracted
Set-Content -LiteralPath (Join-Path $out 'match_rules_original.h') -Encoding ASCII $extracted
$batch = @"
@echo off
call "$VcVars" >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MT /O2 /EHsc /W4 /WX /I"$out" /Fo"$out\test_match_rules.obj" /Fe"$out\test_match_rules.exe" "$root\tests\host\test_match_rules.cpp"
if errorlevel 1 exit /b 1
"$out\test_match_rules.exe"
exit /b %errorlevel%
"@
$batchPath = Join-Path $out 'test.bat'
Set-Content -LiteralPath $batchPath -Encoding ASCII $batch
& cmd.exe /d /c $batchPath
if ($LASTEXITCODE -ne 0) { throw 'Native match rules regression failed.' }
