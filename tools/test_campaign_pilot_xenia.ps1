[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$DiagnosticDir,
      [Parameter(Mandatory=$true)][string]$OutDir,
      [Parameter(Mandatory=$true)][ValidateSet(3,4)][int]$Mode,
      [ValidateRange(0,19)][int]$Round=0,
      [Parameter(Mandatory=$true)][string[]]$Markers,
      [ValidateRange(10,1200)][int]$Seconds=300,
      [ValidateRange(0,30)][int]$Player=18,
      [string]$BuildDir='build-x360/menu-port/full-vs-smoke')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$scope=(Resolve-Path -LiteralPath (Join-Path $root 'build-x360')).Path
$run=(Resolve-Path -LiteralPath $BuildDir).Path
$out=$OutDir
$Name=Split-Path -Leaf $OutDir
if(Test-Path -LiteralPath $out) { throw 'Existing pilot evidence.' }
New-Item -ItemType Directory -Force $out | Out-Null
$out=(Resolve-Path -LiteralPath $out).Path
if(-not $run.StartsWith($scope+'\',[StringComparison]::OrdinalIgnoreCase) -or
   -not $out.StartsWith($scope+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Paths out of test scope.' }
if(Test-Path (Join-Path $run 'runtime-trace.txt')) { throw 'Previous trace not archived.' }
if(Get-CimInstance Win32_Process -Filter "Name='xenia_canary.exe'") { throw 'An existing Xenia process is active.' }
$image=(Resolve-Path -LiteralPath $DiagnosticDir).Path
foreach($taskFile in @('default.xex','menus.map','build-info.json')) {
    Copy-Item -LiteralPath (Join-Path $image $taskFile) -Destination (Join-Path $run $taskFile) -Force
}
$record=Get-Content -Raw (Join-Path $run 'build-info.json') | ConvertFrom-Json
if(-not $record.boot_to_match) { throw 'Pilot requires a BootToMatch diagnostic.' }
if(-not (Test-Path (Join-Path $run 'melee.iso'))) { throw 'Reuse a run directory with an existing disc image.' }
$xex=Join-Path $run 'default.xex'
if((Get-FileHash -LiteralPath $xex).Hash -ne $record.xex_sha256 -or
   (Get-FileHash -LiteralPath (Join-Path $run 'menus.map')).Hash -ne $record.map_sha256) { throw 'Diagnostic hash mismatch.' }
$config=@("mode $Mode","round $Round","p1 $Player",'cpu_level 1','seed 12000','human 1') -join "`n"
[IO.File]::WriteAllText((Join-Path $run 'match-config.txt'),$config+"`n",[Text.Encoding]::ASCII)
[IO.File]::WriteAllText((Join-Path $run 'input-script.txt'),"routepilot`n",[Text.Encoding]::ASCII)
function Read-Shared([string]$path) {
    if(-not (Test-Path -LiteralPath $path)) { return '' }
    $f=[IO.FileStream]::new($path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete)
    try { $reader=[IO.StreamReader]::new($f);return $reader.ReadToEnd() } finally { $f.Dispose() }
}
function Last-Counter([string]$text,[string]$name) {
    $m=[regex]::Matches($text,'(?m)^'+[regex]::Escape($name)+': (\d+)\r?$')
    if($m.Count) { return [long]$m[$m.Count-1].Groups[1].Value };return -1L
}
$p=Start-Process -FilePath (Join-Path $root 'build-x360/xenia-canary/xenia_canary.exe') -WindowStyle Hidden -PassThru `
    -ArgumentList @('--headless=true','--allow_game_relative_writes=true','--discord=false','--apu=xaudio2','--mute=false',
        "--log_file=`"$(Join-Path $run 'xenia.log')`"","`"$xex`"")
$deadline=(Get-Date).AddSeconds($Seconds)
$nextUpdate=(Get-Date).AddSeconds(30)
$trace=Join-Path $run 'runtime-trace.txt'
$text=''
try {
    while((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 1000
        $text=Read-Shared $trace
        $p.Refresh()
        $missing=@($Markers | Where-Object { $text -notmatch ('(?m)^'+[regex]::Escape($_)+'\r?$') })
        if($p.HasExited -or $text -match '(?m)^hang\.' -or $missing.Count -eq 0) { break }
        if((Get-Date) -ge $nextUpdate) {
            Write-Host ("PILOT {0}: loop={1}, round={2}, hits={3}" -f $Name,(Last-Counter $text 'loop.frame'),(Last-Counter $text 'match.campaign.round'),(Last-Counter $text 'loop.match_hits'))
            $nextUpdate=(Get-Date).AddSeconds(30)
        }
    }
} finally {
    $owned=Get-CimInstance Win32_Process -Filter "ProcessId=$($p.Id)"
    if($owned -and $owned.CommandLine.Contains($xex)) { Stop-Process -Id $p.Id -Force;$p.WaitForExit() }
}
$text=Read-Shared $trace
$missing=@($Markers | Where-Object { $text -notmatch ('(?m)^'+[regex]::Escape($_)+'\r?$') })
$result=[ordered]@{
    DiagnosticDir=$image;XexSha256=$record.xex_sha256;MapSha256=$record.map_sha256
    LoopFrame=(Last-Counter $text 'loop.frame');Round=(Last-Counter $text 'match.campaign.round')
    Hits=(Last-Counter $text 'loop.match_hits');DecodeFailures=(Last-Counter $text 'hsd.decode_failures')
    AudioHistoryMismatches=(Last-Counter $text 'audio.history_mismatches');MissingMarkers=$missing
    Watchdog=[bool]($text -match '(?m)^hang\.')
}
$result.Pass=$missing.Count -eq 0 -and -not $result.Watchdog -and $result.DecodeFailures -eq 0 -and
    $result.AudioHistoryMismatches -eq 0 -and (Get-FileHash -LiteralPath $xex).Hash -eq $record.xex_sha256
$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $out 'result.json') -Encoding UTF8
foreach($taskFile in @('runtime-trace.txt','xenia.log','input-script.txt','match-config.txt')) {
    $path=Join-Path $run $taskFile
    if(Test-Path -LiteralPath $path) { Move-Item -LiteralPath $path -Destination (Join-Path $out $taskFile) }
}
[pscustomobject]$result | Format-List
if(-not $result.Pass) { throw 'Pilot objective was not completed; inspect saved evidence.' }
