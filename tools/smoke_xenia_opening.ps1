[CmdletBinding()]
param(
    [string] $XexPath = '',
    [string] $MapPath = '',
    [string] $XeniaPath = '',
    [string] $OutDir = '',
    [ValidateRange(300, 2147483647)] [int] $TargetFrame = 3600,
    [ValidateRange(10, 600)] [int] $Seconds = 110
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $XexPath) { $XexPath = Join-Path $root 'dist/default.xex' }
if (-not $MapPath) { $MapPath = Join-Path $root 'build-x360/xdk/melee360.map' }
if (-not $XeniaPath) { $XeniaPath = Join-Path $root 'build-x360/xenia-canary/xenia_canary.exe' }
if (-not $OutDir) { $OutDir = Join-Path $root 'build-x360/opening-smoke' }
$XexPath = (Resolve-Path -LiteralPath $XexPath).Path
$MapPath = (Resolve-Path -LiteralPath $MapPath).Path
$XeniaPath = (Resolve-Path -LiteralPath $XeniaPath).Path
if ((Get-Item -LiteralPath $MapPath).LastWriteTimeUtc -gt (Get-Item -LiteralPath $XexPath).LastWriteTimeUtc) {
    throw 'Map is newer than XEX; finish the normal build before testing.'
}
if (Get-CimInstance Win32_Process -Filter "Name = 'xenia_canary.exe'") {
    throw 'Close the existing Xenia run before starting the opening smoke.'
}
New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir = (Resolve-Path -LiteralPath $OutDir).Path
$trace = Join-Path (Split-Path -Parent $XexPath) 'runtime-trace.txt'
if (Test-Path -LiteralPath $trace) {
    Copy-Item -LiteralPath $trace -Destination (Join-Path $OutDir 'previous-runtime-trace.txt') -Force
    Remove-Item -LiteralPath $trace
}
$hash = (Get-FileHash -LiteralPath $XexPath).Hash
Copy-Item -LiteralPath $MapPath -Destination (Join-Path $OutDir 'normal.map') -Force
function Read-Shared([string] $Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return '' }
    $fs = [IO.FileStream]::new($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read,
        [IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete)
    try {
        $reader = [IO.StreamReader]::new($fs)
        return $reader.ReadToEnd()
    } finally { $fs.Dispose() }
}
function Last-Counter([string] $Text, [string] $Name) {
    $matches = [regex]::Matches($Text, '(?m)^' + [regex]::Escape($Name) + ': (\d+)\r?$')
    if (-not $matches.Count) { return -1L }
    return [long] $matches[$matches.Count - 1].Groups[1].Value
}
$xlog = Join-Path $OutDir 'xenia.log'
$argsList = @('--headless=true', '--allow_game_relative_writes=true', '--discord=false',
    '--apu=xaudio2', '--mute=false', "--log_file=`"$xlog`"", "`"$XexPath`"")
$p = Start-Process -FilePath $XeniaPath -ArgumentList $argsList -WindowStyle Hidden -PassThru
Set-Content -LiteralPath (Join-Path $OutDir 'owned-pid.txt') -Value $p.Id
$deadline = (Get-Date).AddSeconds($Seconds)
$text = ''
try {
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 1000
        $text = Read-Shared $trace
        $p.Refresh()
        if ($p.HasExited -or $text -match '(?m)^hang\.' -or
            (Last-Counter $text 'loop.frame') -ge $TargetFrame) { break }
    }
} finally {
    # Stop only the process launched here, with its expected executable path.
    $owned = Get-CimInstance Win32_Process -Filter "ProcessId = $($p.Id)"
    if ($owned -and $owned.CommandLine.Contains($XexPath)) {
        Stop-Process -Id $p.Id -Force
        $p.WaitForExit()
    }
    if (Test-Path -LiteralPath $trace) {
        Copy-Item -LiteralPath $trace -Destination (Join-Path $OutDir 'runtime-trace.txt') -Force
    }
}
$text = Read-Shared (Join-Path $OutDir 'runtime-trace.txt')
$result = [ordered]@{
    XexSha256 = $hash
    MapSha256 = (Get-FileHash -LiteralPath (Join-Path $OutDir 'normal.map')).Hash
    LoopFrame = Last-Counter $text 'loop.frame'
    MoviePresented = Last-Counter $text 'movie.presented'
    MovieErrors = Last-Counter $text 'movie.errors'
    AudioHistoryMismatches = Last-Counter $text 'audio.history_mismatches'
    Watchdog = [bool] ($text -match '(?m)^hang\.')
    ScriptLoaded = [bool] ($text -match '(?m)^script.steps:')
}
$result.Pass = $result.LoopFrame -ge $TargetFrame -and $result.MoviePresented -gt 0 -and
    $result.MovieErrors -eq 0 -and $result.AudioHistoryMismatches -eq 0 -and
    -not $result.Watchdog -and -not $result.ScriptLoaded -and
    (Get-FileHash -LiteralPath $XexPath).Hash -eq $hash
$result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutDir 'result.json') -Encoding UTF8
[pscustomobject] $result | Format-List
if (-not $result.Pass) { throw 'Normal opening smoke failed; see the saved trace and result.' }
