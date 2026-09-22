[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$dumpbin = Join-Path $env:XEDK 'bin/win32/dumpbin.exe'
if (!(Test-Path $dumpbin)) { throw 'XDK dumpbin unavailable.' }
$probe = Join-Path $root 'build-x360/gameplay-probe'
$manifestPath = Join-Path $probe 'manifest.json'
if (!(Test-Path $manifestPath)) { throw 'Run tools/probe_gameplay_xdk.ps1 to create a current manifest.' }
$manifest = Get-Content -Raw $manifestPath | ConvertFrom-Json
if (!$manifest.Complete) { throw 'Last gameplay probe did not complete successfully; refusing stale objects.' }
$objects = @($manifest.Objects | ForEach-Object { Get-Item (Join-Path $probe $_.Object) })
if (!$objects.Count) { throw 'Run tools/probe_gameplay_xdk.ps1 first.' }
$definitions = @{}
$references = @{}
# This is an object-level dependency inventory, not a successful linker test.
# Runtime libraries and COMDAT reachability are deliberately not inferred.
foreach ($object in (@(Get-ChildItem "$root/build-x360/xdk" -Filter '*.obj') + $objects)) {
    $lines = & $dumpbin /symbols $object.FullName
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect $($object.FullName)" }
    foreach ($line in $lines) {
        if ($line -notmatch '^\s*[0-9A-F]+\s+[0-9A-F]+\s+(UNDEF|SECT\w+|ABS)\s+.*\bExternal\s+\|\s+(.+)$') { continue }
        $section = $Matches[1]
        $symbol = $Matches[2].Trim()
        if ($section -ne 'UNDEF') { $definitions[$symbol] = $true }
        elseif ($object.DirectoryName -eq $probe) {
            if (!$references.ContainsKey($symbol)) { $references[$symbol] = @() }
            $references[$symbol] += $object.BaseName
        }
    }
}
$missing = @($references.Keys | Where-Object { !$definitions.ContainsKey($_) } | Sort-Object)
$report = @($missing | ForEach-Object {
    [pscustomobject]@{ Symbol = $_; RequiredBy = @($references[$_] | Sort-Object -Unique) }
})
$report | ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 "$probe/unresolved-symbols.json"
Write-Output "Probe objects: $($objects.Count); external symbols absent from probe/runtime objects: $($missing.Count)"
$report | Group-Object { ($_.Symbol -split '_')[0] } | Sort-Object Count -Descending |
    Select-Object -First 20 Count, Name | Format-Table
Write-Output "Full inventory: $probe/unresolved-symbols.json"
