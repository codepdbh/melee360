[CmdletBinding()]
param(
    [ValidatePattern('^v[0-9]+\.[0-9]+\.[0-9]+-alpha\.[0-9]+$')]
    [string] $Version = 'v0.1.0-alpha.1'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$commit = (& git -C $root rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot read source revision.' }
if (& git -C $root status --porcelain --untracked-files=normal) {
    throw 'Commit source and documentation before packaging; checkout must be clean.'
}
$dist = Join-Path $root 'dist'
$info = Get-Content -Raw -LiteralPath (Join-Path $dist 'build-info.json') | ConvertFrom-Json
if ($info.schema -ne 1 -or $info.commit -ne $commit -or $info.dirty -ne $false) {
    throw 'Rebuild from this clean commit before packaging.'
}
foreach ($flag in @('boot_to_match','input_script','call_trace','experimental_race')) {
    if ($info.$flag -ne $false) { throw "Diagnostic flag $flag is not allowed in a playable prerelease." }
}
$upstream = (& git -C (Join-Path $root 'upstream/melee-pc') rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $upstream -ne $info.upstream -or $info.upstream_dirty -ne $false -or
    (& git -C (Join-Path $root 'upstream/melee-pc') status --porcelain --untracked-files=normal)) {
    throw 'Upstream must match the clean revision used for this build.'
}
$xex = Join-Path $dist 'default.xex'
$map = Join-Path $root 'build-x360/xdk/melee360.map'
if ((Get-FileHash -LiteralPath $xex).Hash -ne $info.xex_sha256 -or
    (Get-FileHash -LiteralPath $map).Hash -ne $info.map_sha256) {
    throw 'Executable/map does not match the build record.'
}
if ((Get-Item -LiteralPath $map).LastWriteTimeUtc -gt (Get-Item -LiteralPath $xex).LastWriteTimeUtc) {
    throw 'Map is newer than executable.'
}
$output = Join-Path $root "build-x360/releases/$Version"
if (Test-Path -LiteralPath $output) { throw "Output already exists: $output. Use a new version or inspect it first." }
$stage = Join-Path $output 'MELEE360'
New-Item -ItemType Directory -Path (Join-Path $stage 'assets') -Force | Out-Null
$utf8 = [Text.UTF8Encoding]::new($false)
function Write-Utf8([string] $path, [string] $content) { [IO.File]::WriteAllText($path, $content, $utf8) }
Copy-Item -LiteralPath $xex -Destination (Join-Path $stage 'default.xex')
Copy-Item -LiteralPath (Join-Path $dist 'assets/sprite_atlas.png') -Destination (Join-Path $stage 'assets/sprite_atlas.png')
$guide = [IO.File]::ReadAllText((Join-Path $root 'docs/INSTALAR_XBOX360.md')).Replace('v0.1.0-alpha.1', $Version)
Write-Utf8 (Join-Path $stage 'INSTRUCCIONES.md') $guide
$record = [ordered]@{
    version = $Version; commit = $commit; built_utc = $info.built_utc
    source_url = "https://github.com/codepdbh/melee360/tree/$commit"
    upstream = $upstream; upstream_url = "https://github.com/999sian/melee-pc/tree/$upstream"
    boot_to_match = $false; input_script = $false; call_trace = $false; experimental_race = $false
    xex_sha256 = $info.xex_sha256; map_sha256 = $info.map_sha256
    hardware_verified = $false; full_campaign_verified = $false
}
Write-Utf8 (Join-Path $stage 'BUILD.json') (($record | ConvertTo-Json) + "`n")
$notices = @"
MELEE360 $Version - prerelease experimental
Codigo y estado: https://github.com/codepdbh/melee360/tree/$commit

No incluye ISO, modelos, texturas, sonidos ni otros datos extraidos del disco;
los recursos originales se leen de la imagen GALE01 aportada por el usuario.
sprite_atlas.png es el atlas de interfaz generado por tools/generate_sprite_atlas.ps1.
No incluye SDK, claves, NAND ni archivos de la consola.

Super Smash Bros. Melee es propiedad de Nintendo / HAL Laboratory.
La base reconstruida no ofrece licencia de redistribucion del codigo del juego.
El LICENSE.md de melee-pc distingue ese codigo de sus adiciones GPL y terceros.
No se declara una licencia unica para este ejecutable ni para todo el proyecto.
Condiciones de la base utilizada:
https://github.com/999sian/melee-pc/blob/$upstream/LICENSE.md
Dependencias: https://github.com/codepdbh/melee360/blob/$commit/docs/UPSTREAMS.md

Probado mediante trazas en Xenia; hardware Xbox 360 todavia sin verificar.
Classic/Adventure, colisiones y presentacion permanecen incompletos.
"@
Write-Utf8 (Join-Path $stage 'AVISOS.txt') ($notices + "`n")
# Explicit allowlist: never recurse over dist or copy a disc image into staging.
$payload = @('default.xex','assets/sprite_atlas.png','INSTRUCCIONES.md','AVISOS.txt','BUILD.json')
$hashes = foreach ($relative in $payload) {
    "{0}  {1}" -f (Get-FileHash -LiteralPath (Join-Path $stage $relative)).Hash.ToLowerInvariant(), $relative
}
Write-Utf8 (Join-Path $stage 'SHA256SUMS.txt') (($hashes -join "`n") + "`n")
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zipName = "MELEE360-$Version-xbox360.zip"
$zipPath = Join-Path $output $zipName
$archive = [IO.Compression.ZipFile]::Open($zipPath, [IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($relative in ($payload + 'SHA256SUMS.txt')) {
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive,
            (Join-Path $stage $relative), "MELEE360/$relative", [IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
} finally { $archive.Dispose() }
# Read back every entry and compare bytes, including the checksum file itself.
$archive = [IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $expected = $payload + 'SHA256SUMS.txt'
    if ($archive.Entries.Count -ne $expected.Count) { throw 'Unexpected ZIP entries.' }
    foreach ($relative in $expected) {
        $entry = $archive.GetEntry("MELEE360/$relative")
        if (-not $entry) { throw "Missing ZIP entry: $relative" }
        $stream = $entry.Open(); $sha = [Security.Cryptography.SHA256]::Create()
        try { $actual = ([BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-','') }
        finally { $stream.Dispose(); $sha.Dispose() }
        if ($actual -ne (Get-FileHash -LiteralPath (Join-Path $stage $relative)).Hash) {
            throw "ZIP checksum mismatch: $relative"
        }
    }
} finally { $archive.Dispose() }
$zipHash = (Get-FileHash -LiteralPath $zipPath).Hash.ToLowerInvariant()
Write-Utf8 (Join-Path $output 'SHA256SUMS.txt') ("$zipHash  $zipName`n")
Write-Host "Created and verified: $zipPath"
Write-Host "SHA256: $zipHash"
Write-Host "Size: $((Get-Item -LiteralPath $zipPath).Length) bytes; 6 files; no ISO or diagnostic inputs."
