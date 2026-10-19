[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$BuildDir,
    [Parameter(Mandatory=$true)][string]$ScriptPath,
    [string[]]$Expect=@('menu.fonts.original: 1','match.select.original: 0','match.select.stage.enter: 0','match.enter.fighters: 2'),
    [int]$TargetFrame=3000,
    [ValidateRange(0,2147483647)][int]$MinHits=1,
    [ValidateRange(0,3)][int]$ExpectedFlow=3,
    [ValidateRange(10,600)][int]$Seconds=100
)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$BuildDir=(Resolve-Path -LiteralPath $BuildDir).Path
$xex=Join-Path $BuildDir 'default.xex'
$record=Get-Content -Raw (Join-Path $BuildDir 'build-info.json') | ConvertFrom-Json
if((Get-FileHash $xex).Hash -ne $record.xex_sha256 -or
   (Get-FileHash (Join-Path $BuildDir 'menus.map')).Hash -ne $record.map_sha256) { throw 'Diagnostic hashes mismatch.' }
if(Get-CimInstance Win32_Process -Filter "Name='xenia_canary.exe'") { throw 'Close the existing Xenia run first.' }
$trace=Join-Path $BuildDir 'runtime-trace.txt'
if(Test-Path -LiteralPath $trace) { throw 'Use a fresh diagnostic folder for each smoke run.' }
Copy-Item -LiteralPath $ScriptPath -Destination (Join-Path $BuildDir 'input-script.txt')
if(-not (Test-Path (Join-Path $BuildDir 'melee.iso'))) {
    $iso=@(Get-ChildItem -LiteralPath (Join-Path $root 'iso') -Filter '*.iso' -File)
    if($iso.Count -ne 1) { throw 'Expected one local disc image.' }
    New-Item -ItemType HardLink -Path (Join-Path $BuildDir 'melee.iso') -Target $iso[0].FullName | Out-Null
}
function Read-Shared([string]$path) {
    if(-not (Test-Path -LiteralPath $path)) { return '' }
    $fs=[IO.FileStream]::new($path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete)
    try { $reader=[IO.StreamReader]::new($fs);return $reader.ReadToEnd() } finally { $fs.Dispose() }
}
function Last-Counter([string]$text,[string]$name) {
    $m=[regex]::Matches($text,'(?m)^'+[regex]::Escape($name)+': (\d+)\r?$')
    if($m.Count) { return [long]$m[$m.Count-1].Groups[1].Value }
    return -1L
}
$p=Start-Process -FilePath (Join-Path $root 'build-x360/xenia-canary/xenia_canary.exe') -WindowStyle Hidden -PassThru `
    -ArgumentList @('--headless=true','--allow_game_relative_writes=true','--discord=false','--apu=xaudio2','--mute=false',
        "--log_file=`"$(Join-Path $BuildDir 'xenia.log')`"","`"$xex`"")
Set-Content -LiteralPath (Join-Path $BuildDir 'owned-pid.txt') $p.Id
$deadline=(Get-Date).AddSeconds($Seconds)
try {
    while((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 1000
        $text=Read-Shared $trace
        $p.Refresh()
        if($p.HasExited -or $text -match '(?m)^hang\.' -or (Last-Counter $text 'loop.frame') -ge $TargetFrame) { break }
    }
} finally {
    $owned=Get-CimInstance Win32_Process -Filter "ProcessId=$($p.Id)"
    if($owned -and $owned.CommandLine.Contains($xex)) { Stop-Process -Id $p.Id -Force;$p.WaitForExit() }
}
$text=Read-Shared $trace
$missing=@($Expect | Where-Object { $text -notmatch ('(?m)^'+[regex]::Escape($_)+'\r?$') })
$result=[ordered]@{
    XexSha256=$record.xex_sha256;MapSha256=$record.map_sha256
    LoopFrame=Last-Counter $text 'loop.frame';Hits=Last-Counter $text 'loop.match_hits'
    FlowState=Last-Counter $text 'loop.flow_state'
    AudioHistoryMismatches=Last-Counter $text 'audio.history_mismatches'
    DecodeFailures=Last-Counter $text 'hsd.decode_failures'
    Watchdog=[bool]($text -match '(?m)^hang\.')
    MissingMarkers=$missing
    ScriptSha256=(Get-FileHash -LiteralPath $ScriptPath).Hash
}
$result.Pass=$result.LoopFrame -ge $TargetFrame -and -not $result.Watchdog -and
    $result.AudioHistoryMismatches -eq 0 -and $result.DecodeFailures -eq 0 -and $missing.Count -eq 0 -and
    ($MinHits -eq 0 -or $result.Hits -ge $MinHits) -and
    $result.FlowState -eq $ExpectedFlow -and
    (Get-FileHash $xex).Hash -eq $record.xex_sha256
$result | ConvertTo-Json | Set-Content -Encoding UTF8 (Join-Path $BuildDir 'result.json')
[pscustomobject]$result | Format-List
if(-not $result.Pass) { throw 'Menu smoke failed; inspect result.json and the paired map/trace.' }
