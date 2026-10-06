[CmdletBinding()]
param(
    # Configuration names or wildcards to run (see Get-SoakMatrix); default all.
    [string[]] $Only = @('*'),
    # Rebuild the soak XEX (-BootToMatch -InputScript) before running.
    [switch] $Build,
    # Overrides every configuration's run time.
    [int] $Seconds = 0,
    [ValidateRange(0, 2147483647)]
    [int] $TargetFrame = 0,
    # Output folder for the matrix, traces and logs.
    [string] $OutDir = '',
    # Show the Xenia window instead of running headless.
    [switch] $Visible,
    [switch] $Audio,
    [string] $XeniaPath = ''
)

# Unattended Xenia soak runs over a matrix of match configurations.
#
# One XEX built with -BootToMatch -InputScript reads game:\match-config.txt
# (stage, fighters, players, rules, items, CPU level, repeat count) and
# game:\input-script.txt (P1 input) at runtime, so no rebuild is needed per
# configuration. Runs are strictly sequential (one xenia_canary at a time, any
# leftover instance is killed first) from dist\soak0, which holds the XEX and a
# hardlink to melee.iso. The run is classified from runtime-trace.txt:
#   OK      the target match frame was reached or every repeat finished
#   FREEZE  the in-game watchdog reported hang.* (main loop stalled 3 s)
#   CRASH   Xenia exited early or the trace stopped without a watchdog report;
#           CRASH(host-oom) when Xenia shows its 'Unhandled Exception' box for
#           a host quota/memory failure (the box is closed by killing Xenia)
#   SHORT   still running but below the target match frame
# Xenia's private commit is sampled every 2 s (peak and MB/min after boot) to
# catch guest-driven host leaks. Input is scripted inside the guest; this
# script never sends window input or takes focus.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $XeniaPath) { $XeniaPath = Join-Path $root 'build-x360\xenia-canary\xenia_canary.exe' }
if (-not $OutDir) { $OutDir = Join-Path $root 'build-x360\soak' }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$dist = Join-Path $root 'dist'
$iso = Join-Path $dist 'melee.iso'
$soakXex = Join-Path $root 'build-x360\soak.xex'

$Fighters = @('Mario', 'Luigi', 'DrMario', 'Peach', 'Yoshi', 'Bowser', 'DK', 'Falcon', 'Ganondorf',
    'Fox', 'Falco', 'Link', 'YoungLink', 'Zelda', 'Sheik', 'Samus', 'Pikachu', 'Pichu', 'Jigglypuff',
    'Ness', 'Marth', 'Roy', 'Mewtwo', 'GameWatch', 'Kirby', 'IceClimbers')
$Stages = @('Battlefield', 'FinalDestination', 'DreamLand', 'YoshisStory', 'FountainOfDreams',
    'YoshisIsland64', 'HyruleTemple', 'KongoJungle64', 'JungleJapes')

$LandingScript = @'
250 0 0 0 0 -
15 0 0 0 0 X
4 0 0 0 0 -
3 0 127 0 0 B
300 0 0 0 0 -
40 30 0 0 0 -
40 -30 0 0 0 -
60 0 0 0 0 L
100 0 0 0 0 -
loop
'@

# P1 moveset: waits out the countdown, then ground attacks, tilts, smashes,
# aerials, the four specials, shield, rolls, spot/air dodges, grabs with all
# four throws and a taunt, and loops. Format: frames stickX stickY cX cY buttons.
$MovesetScript = @'
# countdown
250 0 0 0 0 -
# jab, tilts
3 0 0 0 0 A
25 0 0 0 0 -
4 60 0 0 0 A
30 0 0 0 0 -
4 0 60 0 0 A
30 0 0 0 0 -
4 0 -60 0 0 A
30 0 0 0 0 -
# smashes (C-stick)
3 0 0 127 0 -
50 0 0 0 0 -
3 0 0 0 127 -
50 0 0 0 0 -
3 0 0 0 -127 -
50 0 0 0 0 -
# dash attack
12 127 0 0 0 -
3 127 0 0 0 A
40 0 0 0 0 -
# aerials
3 0 0 0 0 X
6 0 0 0 0 -
3 0 0 0 0 A
45 0 0 0 0 -
3 0 0 0 0 X
6 0 0 0 0 -
3 0 0 127 0 -
45 0 0 0 0 -
3 0 0 0 0 X
6 0 0 0 0 -
3 0 0 -127 0 -
45 0 0 0 0 -
3 0 0 0 0 X
6 0 0 0 0 -
3 0 0 0 127 -
45 0 0 0 0 -
3 0 0 0 0 X
6 0 0 0 0 -
3 0 0 0 -127 -
45 0 0 0 0 -
# specials: neutral, side, up, down
3 0 0 0 0 B
70 0 0 0 0 -
3 -127 0 0 0 B
70 0 0 0 0 -
3 0 127 0 0 B
100 0 0 0 0 -
3 0 -127 0 0 B
70 0 0 0 0 -
# shield, rolls, spot dodge
30 0 0 0 0 R
3 127 0 0 0 R
40 0 0 0 0 -
10 0 0 0 0 R
3 -127 0 0 0 R
40 0 0 0 0 -
10 0 0 0 0 R
3 0 -127 0 0 R
40 0 0 0 0 -
# air dodge
3 0 0 0 0 X
8 0 0 0 0 -
3 60 -60 0 0 R
60 0 0 0 0 -
# grabs: pummel, forward, back, up, down throw
3 0 0 0 0 Z
12 0 0 0 0 -
3 0 0 0 0 A
10 0 0 0 0 -
3 127 0 0 0 -
60 0 0 0 0 -
3 0 0 0 0 Z
12 0 0 0 0 -
3 -127 0 0 0 -
60 0 0 0 0 -
3 0 0 0 0 Z
12 0 0 0 0 -
3 0 127 0 0 -
60 0 0 0 0 -
3 0 0 0 0 Z
12 0 0 0 0 -
3 0 -127 0 0 -
60 0 0 0 0 -
# taunt, walk back toward the other side
3 0 0 0 0 u
90 0 0 0 0 -
40 -90 0 0 0 -
20 0 0 0 0 -
loop
'@

$LifecycleScript = @'
250 0 0 0 0 -
240 127 0 0 0 -
90 0 -127 0 0 -
60 0 0 0 0 -
loop
'@

$RouteScript = @'
4 127 0 0 0 X
65 127 0 0 0 -
4 127 0 0 0 A
30 127 0 0 0 -
loop
'@

function New-Config([string] $Name, [hashtable] $Values) {
    $c = [ordered]@{ Name = $Name; Stage = 0; P1 = 0; P2 = 0; P3 = -1; P4 = -1; Players = 2; Human = 1;
        CpuLevel = 9; Stocks = 2; Time = 0; Items = -1; Repeat = 1; Seconds = 120; TargetFrame = 5400; Script = 'moveset'; Mode = 2; Round = 0 }
    foreach ($k in $Values.Keys) { $c[$k] = $Values[$k] }
    [pscustomobject] $c
}

function Get-SoakMatrix {
    $list = @()
    # Every fighter once as P1 (scripted moveset) and once as the CPU, on a
    # rotating stage, with items on every other run.
    for ($i = 0; $i -lt $Fighters.Count; ++$i) {
        $cpu = ($i + 13) % $Fighters.Count
        $stage = $i % $Stages.Count
        $items = if ($i % 2) { 2 } else { -1 }
        $list += New-Config ("p1-{0}-vs-{1}-{2}" -f $Fighters[$i], $Fighters[$cpu], $Stages[$stage]) @{
            P1 = $i; P2 = $cpu; Stage = $stage; Items = $items }
    }
    # Every stage with four CPUs and items (free-for-all).
    for ($s = 0; $s -lt $Stages.Count; ++$s) {
        $a = ($s * 3) % $Fighters.Count
        $list += New-Config ("ffa4-{0}" -f $Stages[$s]) @{
            Stage = $s; Players = 4; Human = 0; P1 = $a; P2 = ($a + 7) % 26; P3 = ($a + 14) % 26; P4 = ($a + 21) % 26
            Items = 3; Stocks = 2; Seconds = 150 }
    }
    # One-minute timed match between level-1 CPUs: usually a tie, so sudden death.
    $list += New-Config 'timed-1min-sudden-death' @{ Human = 0; CpuLevel = 1; Time = 1; P1 = 0; P2 = 9; Seconds = 150; TargetFrame = 99999 }
    # Three chained one-stock matches: teardown and heap reuse.
    $list += New-Config 'repeat-3-matches' @{ Human = 0; Stocks = 1; Repeat = 3; P1 = 9; P2 = 20; Items = 2; Seconds = 240; TargetFrame = 99999 }
    $list += New-Config 'repeat-3-scripted-falls' @{ Human = 1; CpuLevel = 1; Stocks = 1; Repeat = 3; P1 = 9; P2 = 20; Items = -1; Seconds = 150; TargetFrame = 99999; Script = 'lifecycle' }
    for ($round = 0; $round -lt 20; ++$round) {
        $list += New-Config ("adventure-{0:D2}" -f $round) @{ Mode = 4; Round = $round; P1 = 0; TargetFrame = 1800; Seconds = 70 }
    }
    $list += New-Config 'adventure-route-traversal' @{ Mode = 4; Round = 0; P1 = 18; Script = 'route'; TargetFrame = 99999; Seconds = 180 }
    $list += New-Config 'adventure-route-pilot' @{ Mode = 4; Round = 0; P1 = 18; CpuLevel = 3; Script = 'routepilot'; TargetFrame = 99999; Seconds = 240 }
    $list += New-Config 'adventure-route-pilot-hard' @{ Mode = 4; Round = 0; P1 = 18; CpuLevel = 9; Script = 'routepilot'; TargetFrame = 99999; Seconds = 300 }
    $list += New-Config 'adventure-maze-pilot' @{ Mode = 4; Round = 4; P1 = 18; CpuLevel = 3; Script = 'mazepilot'; TargetFrame = 99999; Seconds = 520 }
    $list += New-Config 'adventure-escape-pilot' @{ Mode = 4; Round = 7; P1 = 18; CpuLevel = 3; Script = 'escapepilot'; TargetFrame = 99999; Seconds = 160 }
    $list += New-Config 'adventure-full-pilot' @{ Mode = 4; Round = 0; P1 = 18; CpuLevel = 3; Script = 'adventurepilot'; TargetFrame = 99999; Seconds = 1800 }
    $list += New-Config 'adventure-kirby-wave-pilot' @{ Mode = 4; Round = 9; P1 = 18; CpuLevel = 3; Script = 'wavepilot'; TargetFrame = 99999; Seconds = 240 }
    $list += New-Config 'adventure-pokemon-wave-pilot' @{ Mode = 4; Round = 13; P1 = 18; CpuLevel = 3; Script = 'wavepilot'; TargetFrame = 99999; Seconds = 240 }
    $list += New-Config 'adventure-wireframe-wave-pilot' @{ Mode = 4; Round = 16; P1 = 18; CpuLevel = 3; Script = 'wavepilot'; TargetFrame = 99999; Seconds = 240 }
    $list += New-Config 'adventure-bowser-easy-final-pilot' @{ Mode = 4; Round = 18; P1 = 18; CpuLevel = 3; Script = 'adventurepilot'; TargetFrame = 99999; Seconds = 240 }
    $list += New-Config 'adventure-giga-entry-pilot' @{ Mode = 4; Round = 18; P1 = 18; CpuLevel = 5; Script = 'gigapilot'; TargetFrame = 99999; Seconds = 240 }
    $list += New-Config 'animation-lengths-peach' @{ P1 = 3; P2 = 16; Stage = 3; Stocks = 8; Seconds = 75; TargetFrame = 1800 }
    $list += New-Config 'reserved-parts-young-link' @{ P1 = 12; P2 = 24; Stage = 2; Stocks = 8; Seconds = 110; TargetFrame = 3600 }
    $list += New-Config 'animation-lengths-link' @{ P1 = 11; P2 = 24; Stage = 2; Stocks = 8; Seconds = 75; TargetFrame = 1800 }
    $list += New-Config 'special-landing-falcon' @{ Mode = 4; Round = 7; P1 = 7; CpuLevel = 3; Script = 'landing'; TargetFrame = 1200; Seconds = 50 }
    $list
}

function Write-RunFiles([string] $Dir, $Config) {
    $kinds = @($Config.P1, $Config.P2, $Config.P3, $Config.P4) | ForEach-Object { if ($_ -lt 0) { 0 } else { $_ } }
    $text = @(
        "# $($Config.Name)",
        "mode $($Config.Mode)", "round $($Config.Round)",
        "stage $($Config.Stage)", "p1 $($kinds[0])", "p2 $($kinds[1])", "p3 $($kinds[2])", "p4 $($kinds[3])",
        "players $($Config.Players)", "human $($Config.Human)", "cpu_level $($Config.CpuLevel)",
        "stocks $($Config.Stocks)", "time $($Config.Time)", "items $($Config.Items)", "repeat $($Config.Repeat)", '') -join "`n"
    [IO.File]::WriteAllText((Join-Path $Dir 'match-config.txt'), $text)
    $inputScript = if ($Config.Script -eq 'lifecycle') { $LifecycleScript } elseif ($Config.Script -eq 'route') { $RouteScript } elseif ($Config.Script -eq 'landing') { $LandingScript } elseif ($Config.Script -in @('routepilot','escapepilot','adventurepilot','wavepilot','gigapilot','mazepilot')) { 'routepilot' } else { $MovesetScript }
    [IO.File]::WriteAllText((Join-Path $Dir 'input-script.txt'), $inputScript.Replace("`r", ''))
    Remove-Item -Force -ErrorAction SilentlyContinue (Join-Path $Dir 'runtime-trace.txt')
}

function Initialize-Slot([int] $Index) {
    $dir = Join-Path $dist "soak$Index"
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    $link = Join-Path $dir 'melee.iso'
    if (-not (Test-Path $link)) {
        cmd /c mklink /H "`"$link`"" "`"$iso`"" | Out-Null
        if (-not (Test-Path $link)) { throw "could not hardlink $iso into $dir" }
    }
    $assets = Join-Path $dist 'assets'
    if ((Test-Path $assets) -and -not (Test-Path (Join-Path $dir 'assets'))) {
        Copy-Item -Recurse $assets (Join-Path $dir 'assets')
    }
    Copy-Item -Force $soakXex (Join-Path $dir 'default.xex')
    $dir
}

# Xenia keeps the trace open while writing, so read it with shared access.
function Read-Shared([string] $Path) {
    for ($i = 0; $i -lt 5; ++$i) {
        try {
            $fs = [IO.FileStream]::new($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete)
            try { return ([IO.StreamReader]::new($fs)).ReadToEnd() } finally { $fs.Dispose() }
        } catch [IO.FileNotFoundException] { return '' } catch { Start-Sleep -Milliseconds 200 }
    }
    return ''
}

function Read-Trace([string] $Path) {
    $gigaPending = $false
    $r = [ordered]@{ Lines = 0; MatchFrame = 0; LoopFrame = 0; Hang = $false; AllDone = $false; MatchesDone = 0
        Winner = ''; PlayerWon = $false; GigaEntered = $false; AdventureGoal = $false; EscapeGoal = $false; CampaignComplete = $false; MazeGoal = $false; SuddenDeath = $false; TimeOver = $false; FrameAvg = 0; FrameMax = 0; Over20 = 0; WorkAvg = 0; WorkMax = 0
        HeapUsed = @(); HeapLargest = 0; Hits = 0; Blast = 0; HangCrumb = ''; HangProc = ''; HangStack = @(); HangCalls = @(); Fighters = 0 }
    if (-not (Test-Path $Path)) { return [pscustomobject] $r }
    $lines = @((Read-Shared $Path) -split "`r?`n" | Where-Object { $_ })
    $r.Lines = $lines.Count
    foreach ($line in $lines) {
        $k, $v = $line -split ': ', 2
        if ($null -eq $v) { continue }
        switch -regex ($k) {
            '^loop\.match_frame$' { $r.MatchFrame = [math]::Max($r.MatchFrame, [int64] $v) }
            '^loop\.frame$' { $r.LoopFrame = [int64] $v }
            '^loop\.frame_us_avg$' { $r.FrameAvg = [int64] $v }
            '^loop\.frame_us_max$' { $r.FrameMax = [int64] $v }
            '^loop\.frames_over_20ms$' { $r.Over20 = [int64] $v }
            '^loop\.work_us_avg$' { $r.WorkAvg = [math]::Max($r.WorkAvg, [int64] $v) }
            '^loop\.work_us_max$' { $r.WorkMax = [math]::Max($r.WorkMax, [int64] $v) }
            # A campaign can enter its next match before the harness samples
            # the goal marker. Preserve the same per-match peak as MatchFrame.
            '^loop\.match_hits$' { $r.Hits = [math]::Max($r.Hits, [int64] $v) }
            '^match\.blast_zone\.fighter$' { $r.Blast++ }
            '^match\.enter\.fighters$' { $r.Fighters = [int64] $v; if ($gigaPending -and $r.Fighters -ge 2) { $r.GigaEntered = $true } }
            '^match\.result\.winner$' { $r.Winner = $v; if ($v -eq '0') { $r.PlayerWon = $true } }
            '^match\.adventure\.scene$' { $gigaPending = $v -eq '92' }
            '^match\.adventure\.goal$' { $r.AdventureGoal = $true }
            '^match\.adventure\.escape_goal$' { $r.EscapeGoal = $true }
            '^mode\.preview_complete$' { $r.CampaignComplete = $true }
            '^match\.adventure\.maze_goal$' { $r.MazeGoal = $true }
            '^match\.sudden_death$' { $r.SuddenDeath = $true }
            '^match\.time\.over$' { $r.TimeOver = $true }
            '^auto\.match\.done$' { $r.MatchesDone = [int64] $v }
            '^auto\.all\.done$' { $r.AllDone = $true }
            '^(auto|match)\.heap\.used$' { $r.HeapUsed += [int64] $v }
            '^match\.heap\.largest_free$' { $r.HeapLargest = [int64] $v }
            '^hang\.frame$' { $r.Hang = $true }
            '^hang\.crumb$' { if (-not $r.HangCrumb) { $r.HangCrumb = $v } }
            '^hang\.proc$' { if (-not $r.HangProc) { $r.HangProc = ('0x{0:X8}' -f [int64] $v) } }
            '^hang\.call$' { if ($r.HangCalls.Count -lt 64) { $r.HangCalls += ('0x{0:X8}' -f [int64] $v) } }
            '^hang\.stack\.code$' { if ($r.HangStack.Count -lt 12) { $r.HangStack += ('0x{0:X8}' -f [int64] $v) } }
        }
    }
    [pscustomobject] $r
}

function Get-Verdict($Config, $Trace, [bool] $Exited, [bool] $Stalled) {
    if ($Trace.Hang) { return 'FREEZE' }
    if ($Config.Script -like 'route*' -and $Trace.AdventureGoal) { return 'OK' }
    if ($Config.Script -eq 'escapepilot' -and $Trace.EscapeGoal) { return 'OK' }
    if ($Config.Script -eq 'adventurepilot' -and $Trace.CampaignComplete) { return 'OK' }
    if ($Config.Script -eq 'mazepilot' -and $Trace.MazeGoal) { return 'OK' }
    if ($Config.Script -eq 'wavepilot' -and $Trace.PlayerWon) { return 'OK' }
    if ($Config.Script -eq 'gigapilot' -and $Trace.GigaEntered) { return 'OK' }
    if ($Config.Repeat -gt 1 -and $Trace.AllDone) { return 'OK' }
    if ($Config.Repeat -le 1 -and ($Trace.MatchFrame -ge $Config.TargetFrame -or $Trace.MatchesDone -ge 1)) { return 'OK' }
    if ($Exited) { return 'CRASH' }
    if ($Stalled) { return 'CRASH' }
    return 'SHORT'
}

# Symbols for watchdog addresses, from the linker map of the soak build.
$script:MapSymbols = $null
function Resolve-Address([string] $Hex) {
    if (-not $script:MapSymbols) {
        $map = Join-Path $root 'build-x360\soak.map'
        if (-not (Test-Path $map)) { $map = Join-Path $root 'build-x360\xdk\melee360.map' }
        $list = [Collections.Generic.List[object]]::new()
        if (Test-Path $map) {
            foreach ($l in [IO.File]::ReadAllLines($map)) {
                if ($l -match '^\s*0003:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s+f\s') {
                    $list.Add([pscustomobject] @{ Name = $Matches[1]; Addr = [Convert]::ToUInt32($Matches[2], 16) })
                }
            }
        }
        $script:MapSymbols = @($list | Sort-Object Addr)
    }
    $a = [Convert]::ToUInt32($Hex.Substring(2), 16)
    $lo = 0; $hi = $script:MapSymbols.Count - 1; $best = $null
    while ($lo -le $hi) {
        $mid = [int][math]::Floor(($lo + $hi) / 2)
        if ($script:MapSymbols[$mid].Addr -le $a) { $best = $script:MapSymbols[$mid]; $lo = $mid + 1 } else { $hi = $mid - 1 }
    }
    if ($best) { return ('{0}+0x{1:X}' -f $best.Name, ($a - $best.Addr)) }
    $Hex
}

if ($Build -or -not (Test-Path $soakXex)) {
    Write-Host '[M360][SOAK] building the -BootToMatch -InputScript XEX'
    $log = Join-Path $OutDir 'build.log'
    cmd /c "powershell -NoProfile -ExecutionPolicy Bypass -File `"$root\tools\build_xex.ps1`" -BootToMatch -InputScript -CallTrace > `"$log`" 2>&1"
    if (-not (Select-String -Path $log -Pattern 'created .*default.xex' -Quiet)) { throw "soak build failed, see $log" }
    Copy-Item -Force (Join-Path $dist 'default.xex') $soakXex
    Copy-Item -Force (Join-Path $root 'build-x360\xdk\melee360.map') (Join-Path $root 'build-x360\soak.map')
    Write-Host '[M360][SOAK] note: dist\default.xex is now the soak build; rebuild normally afterwards'
}

$Only = @($Only | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })
$matrix = @(Get-SoakMatrix | Where-Object { $n = $_.Name; @($Only | Where-Object { $n -like $_ }).Count -gt 0 })
if (-not $matrix.Count) { throw "no configuration matches $($Only -join ', ')" }
if ($TargetFrame) {
    foreach ($config in $matrix) { $config.TargetFrame = $TargetFrame }
}
Write-Host "[M360][SOAK] $($matrix.Count) configuration(s), one Xenia instance at a time"
Write-Host "[M360][SOAK] audio enabled: $($Audio.IsPresent)"

# Top-level windows of a process (Xenia's "Unhandled Exception in Xenia"
# error box included) and their static text, without touching focus/input.
if (-not ('M360Soak.Win' -as [type])) {
    Add-Type -Namespace M360Soak -Name Win -MemberDefinition @'
public delegate bool EnumProc(System.IntPtr hwnd, System.IntPtr lparam);
[System.Runtime.InteropServices.DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, System.IntPtr lparam);
[System.Runtime.InteropServices.DllImport("user32.dll")] public static extern bool EnumChildWindows(System.IntPtr parent, EnumProc cb, System.IntPtr lparam);
[System.Runtime.InteropServices.DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(System.IntPtr hwnd, out uint pid);
[System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)] public static extern int GetWindowText(System.IntPtr hwnd, System.Text.StringBuilder text, int max);
[System.Runtime.InteropServices.DllImport("user32.dll")] public static extern bool IsWindowVisible(System.IntPtr hwnd);
public static System.Collections.Generic.List<string> Titles(uint pid) {
    var list = new System.Collections.Generic.List<string>();
    EnumWindows((h, l) => {
        uint owner; GetWindowThreadProcessId(h, out owner);
        if (owner == pid && IsWindowVisible(h)) {
            var sb = new System.Text.StringBuilder(512); GetWindowText(h, sb, 512);
            var title = sb.ToString();
            if (title.Length > 0) {
                var body = new System.Text.StringBuilder();
                EnumChildWindows(h, (c, l2) => { var s = new System.Text.StringBuilder(1024); GetWindowText(c, s, 1024); if (s.Length > 0) body.Append(s.ToString().Replace("\r", " ").Replace("\n", " ")).Append(" | "); return true; }, System.IntPtr.Zero);
                list.Add(title + " :: " + body.ToString());
            }
        }
        return true;
    }, System.IntPtr.Zero);
    return list;
}
'@
}

function Stop-AllXenia {
    for ($i = 0; $i -lt 40; ++$i) {
        $procs = @(Get-CimInstance Win32_Process -Filter "Name = 'xenia_canary.exe'" |
            Where-Object { $_.CommandLine -like '*\dist\soak0\default.xex*' } |
            ForEach-Object { Get-Process -Id $_.ProcessId -ErrorAction SilentlyContinue })
        if (-not $procs.Count) { return }
        $procs | Stop-Process -Force -ErrorAction SilentlyContinue
        Start-Sleep -Milliseconds 500
    }
    throw 'xenia_canary is still running after 20 s'
}

function Get-FreeCommitMB {
    try { [int]((Get-CimInstance Win32_OperatingSystem).FreeVirtualMemory / 1KB) } catch { -1 }
}

$runDirXex = Initialize-Slot 0
$results = @()
foreach ($config in $matrix) {
    Stop-AllXenia
    Write-RunFiles $runDirXex $config
    $runDir = Join-Path $OutDir $config.Name
    New-Item -ItemType Directory -Force -Path $runDir | Out-Null
    $xlog = Join-Path $runDir 'xenia.log'
    Remove-Item -Force -ErrorAction SilentlyContinue $xlog, (Join-Path $runDir 'runtime-trace.txt')
    $freeBefore = Get-FreeCommitMB
    $argsList = @('--allow_game_relative_writes=true', '--discord=false', "--mute=$($(-not $Audio.IsPresent).ToString().ToLowerInvariant())",
        "--log_file=`"$xlog`"", "`"$(Join-Path $runDirXex 'default.xex')`"")
    if ($Audio) { $argsList = @('--apu=xaudio2') + $argsList }
    if (-not $Visible) { $argsList = @('--headless=true') + $argsList }
    $windowStyle = if ($Visible) { 'Normal' } else { 'Hidden' }
    $p = Start-Process -FilePath $XeniaPath -ArgumentList $argsList -PassThru -WindowStyle $windowStyle
    $start = Get-Date
    $secs = if ($Seconds) { $Seconds } else { $config.Seconds }
    $deadline = $start.AddSeconds($secs + 25)
    Write-Host ("[M360][SOAK] start {0} (pid {1}, free commit {2} MB)" -f $config.Name, $p.Id, $freeBefore)
    $tracePath = Join-Path $runDirXex 'runtime-trace.txt'
    $lastLines = 0; $lastChange = Get-Date
    $exited = $false; $stalled = $false; $dialog = ''
    $commitPeak = 0; $commitSamples = [Collections.Generic.List[object]]::new()
    for (;;) {
        Start-Sleep -Seconds 2
        $p.Refresh()
        $exited = $p.HasExited
        if (-not $exited) {
            $mb = [int]($p.PrivateMemorySize64 / 1MB)
            if ($mb -gt $commitPeak) { $commitPeak = $mb }
            $commitSamples.Add([pscustomobject] @{ T = ((Get-Date) - $start).TotalSeconds; MB = $mb })
            $windows = @([M360Soak.Win]::Titles([uint32] $p.Id))
            $err = $windows | Where-Object { $_ -match 'Unhandled Exception|Error|Fatal' } | Select-Object -First 1
            if ($err) { $dialog = $err; break }
        }
        $text = if (Test-Path $tracePath) { Read-Shared $tracePath } else { '' }
        $lines = $text.Length
        if ($lines -ne $lastLines) { $lastLines = $lines; $lastChange = Get-Date }
        $hangDone = ($text -split 'hang\.detail').Count -ge 5
        $finished = ($config.Repeat -gt 1 -and $text -match 'auto\.all\.done') -or
                    ($config.Repeat -le 1 -and $text -match 'auto\.match\.done')
        if ($config.Script -like 'route*' -and $text -match '(?m)^match\.adventure\.goal:') { $finished = $true }
        if ($config.Script -eq 'escapepilot' -and $text -match '(?m)^match\.adventure\.escape_goal:') { $finished = $true }
        if ($config.Script -eq 'adventurepilot' -and $text -match '(?m)^mode\.preview_complete:') { $finished = $true }
        if ($config.Script -eq 'mazepilot' -and $text -match '(?m)^match\.adventure\.maze_goal:') { $finished = $true }
        if ($config.Script -eq 'wavepilot' -and $text -match '(?m)^match\.result\.winner: 0\s*$') { $finished = $true }
        if ($config.Script -eq 'gigapilot' -and $text -match '(?ms)^match\.adventure\.scene: 92\s*\r?\n.*?^match\.enter\.fighters: [2-4]\s*$') { $finished = $true }
        if ($TargetFrame -and $config.Repeat -le 1) {
            $frameSamples = [regex]::Matches($text, '(?m)^loop\.match_frame:\s*(\d+)')
            if ($frameSamples.Count -and [int] $frameSamples[$frameSamples.Count - 1].Groups[1].Value -ge $config.TargetFrame) {
                $finished = $true
            }
        }
        # The main loop traces every 300 frames, so silence means the guest
        # stopped without a watchdog report.
        $quiet = if ($text -match 'loop\.frame') { 45 } else { 120 }
        $stalled = ((Get-Date) - $lastChange).TotalSeconds -gt $quiet
        if ($exited -or $hangDone -or $finished -or $stalled -or (Get-Date) -gt $deadline) { break }
    }
    Stop-AllXenia
    if (Test-Path $tracePath) { Copy-Item -Force $tracePath (Join-Path $runDir 'runtime-trace.txt') }
    $t = Read-Trace (Join-Path $runDir 'runtime-trace.txt')
    $verdict = Get-Verdict $config $t $exited $stalled
    if ($dialog) { $verdict = if ($dialog -match 'Quota|memory') { 'CRASH(host-oom)' } else { 'CRASH(host)' } }
    $guest = $dialog
    if (-not $guest -and (Test-Path $xlog)) {
        $hit = Select-String -Path $xlog -Pattern 'Unhandled|guest crash|Guest crashed|access violation|Cheap-skate exit' |
            Where-Object { $_.Line -notmatch 'exception_addresses|mmio_access_exceptions|RtlRaiseException' } | Select-Object -First 1
        if ($hit) { $guest = $hit.Line.Trim() }
    }
    $stack = @($t.HangStack | ForEach-Object { Resolve-Address $_ })
    if ($t.HangCalls.Count) {
        # Most recent function entries first, consecutive repeats collapsed.
        $calls = [Collections.Generic.List[string]]::new()
        foreach ($c in ($t.HangCalls[($t.HangCalls.Count - 1)..0])) {
            $n = (Resolve-Address $c) -replace '\+0x[0-9A-F]+$', ''
            if (-not $calls.Count -or $calls[$calls.Count - 1] -ne $n) { $calls.Add($n) }
            if ($calls.Count -ge 16) { break }
        }
        $stack = @($calls)
    }
    # Host commit growth over the fight (after the first 30 s of boot/load).
    $late = @($commitSamples | Where-Object { $_.T -ge 30 })
    $growth = 0
    if ($late.Count -ge 2 -and ($late[-1].T - $late[0].T) -gt 10) {
        $growth = [math]::Round(($late[-1].MB - $late[0].MB) / (($late[-1].T - $late[0].T) / 60.0), 1).ToString([Globalization.CultureInfo]::InvariantCulture)
    }
    $heap = if ($t.HeapUsed.Count) { '{0}..{1}' -f ($t.HeapUsed | Measure-Object -Minimum).Minimum, ($t.HeapUsed | Measure-Object -Maximum).Maximum } else { '' }
    $row = [pscustomobject] @{
        Name = $config.Name; Verdict = $verdict; MatchFrame = $t.MatchFrame; Target = $config.TargetFrame; CpuLevel = $config.CpuLevel
        Fighters = $t.Fighters; Hits = $t.Hits; KOs = $t.Blast; Winner = $t.Winner; AdventureGoal = $t.AdventureGoal; EscapeGoal = $t.EscapeGoal; CampaignComplete = $t.CampaignComplete; MazeGoal = $t.MazeGoal; SuddenDeath = $t.SuddenDeath; Matches = $t.MatchesDone
        FrameUsAvg = $t.FrameAvg; FrameUsMax = $t.FrameMax; Over20ms = $t.Over20; WorkUsAvgMax = $t.WorkAvg; WorkUsMax = $t.WorkMax
        HeapUsed = $heap; LargestFree = $t.HeapLargest; CommitPeakMB = $commitPeak; CommitMBPerMin = $growth; FreeCommitMB = $freeBefore
        HangCrumb = $t.HangCrumb; HangProc = $(if ($t.HangProc) { Resolve-Address $t.HangProc } else { '' }); HangStack = ($stack -join ' < ')
        Guest = $guest; Seconds = [int]((Get-Date) - $start).TotalSeconds }
    $results += $row
    Write-Host ("[M360][SOAK] {0,-6} {1} frame={2} hits={3} kos={4} avg={5}us commit={6}MB ({7} MB/min)" -f $verdict, $row.Name, $t.MatchFrame, $t.Hits, $t.Blast, $t.FrameAvg, $commitPeak, $growth)
    if ($verdict -eq 'FREEZE') { Write-Host "    proc=$($row.HangProc) calls=$($row.HangStack)" }
    if ($guest) { Write-Host "    $guest" }
}
Stop-AllXenia

$csv = Join-Path $OutDir 'soak-matrix.csv'
$results | Export-Csv -NoTypeInformation -Path $csv
$md = @('| Config | Verdict | Match frame | Hits | KOs | Winner | Frame us avg/max | >20ms | Heap used | Commit peak MB (MB/min) | Hang / error |',
    '|---|---|---|---|---|---|---|---|---|---|---|')
foreach ($r in $results) {
    $hang = if ($r.Verdict -eq 'FREEZE') { "$($r.HangProc) $($r.HangStack)" } elseif ($r.Guest) { $r.Guest } else { '' }
    $md += "| $($r.Name) | $($r.Verdict) | $($r.MatchFrame) | $($r.Hits) | $($r.KOs) | $($r.Winner) | $($r.FrameUsAvg)/$($r.FrameUsMax) | $($r.Over20ms) | $($r.HeapUsed) | $($r.CommitPeakMB) ($($r.CommitMBPerMin)) | $hang |"
}
$mdPath = Join-Path $OutDir 'soak-matrix.md'
[IO.File]::WriteAllText($mdPath, ($md -join "`n") + "`n")
$summary = $results | Group-Object Verdict | ForEach-Object { "$($_.Name)=$($_.Count)" }
Write-Host "[M360][SOAK] $($summary -join ' ')  ->  $mdPath"
