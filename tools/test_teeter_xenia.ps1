[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$DiagnosticDir,
      [Parameter(Mandatory=$true)][string]$OutDir,
      [string]$IsoPath='dist/melee.iso')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$diagnostic=(Resolve-Path -LiteralPath $DiagnosticDir).Path
$iso=(Resolve-Path -LiteralPath $IsoPath).Path
if(Test-Path -LiteralPath $OutDir) { throw 'Use a fresh output directory.' }
$out=New-Item -ItemType Directory -Path $OutDir
$record=Get-Content -Raw (Join-Path $diagnostic 'build-info.json') | ConvertFrom-Json
if(-not $record.boot_to_match) { throw 'This test needs a boot-to-match diagnostic.' }
foreach($name in @('default.xex','menus.map','build-info.json')) {
    Copy-Item -LiteralPath (Join-Path $diagnostic $name) -Destination $out.FullName
}
New-Item -ItemType HardLink -Path (Join-Path $out.FullName 'melee.iso') -Target $iso | Out-Null
@('mode 2','stage 1','players 2','human 1','cpu_level 1','stocks 8','items -1','p1 0','p2 0','seed 12000') |
    Set-Content -Encoding ASCII (Join-Path $out.FullName 'match-config.txt')
$source=Get-Content -Raw (Join-Path $root 'upstream/melee-pc/src/melee/ft/kinds/ftCommon/forward.h')
$enum=[regex]::Match($source,'(?s)typedef enum ftCommon_MotionState \{.*?\}')
if(-not $enum.Success) { throw 'Original fighter state enum missing.' }
$names=@([regex]::Matches($enum.Value,'\bftCo_MS_\w+') | ForEach-Object {$_.Value})
$enter=[Array]::IndexOf($names,'ftCo_MS_Ottotto')-1
$wait=[Array]::IndexOf($names,'ftCo_MS_OttottoWait')-1
if($enter -lt 0 -or $wait -lt 0) { throw 'Original teeter states missing.' }
& (Join-Path $PSScriptRoot 'smoke_xenia_menu.ps1') -BuildDir $out.FullName `
    -ScriptPath (Join-Path $root 'tests/fixtures/match_teeter_original.txt') `
    -Expect @('auto.stage: 1','match.enter.fighters: 2',"fighter.p1.motion: $enter","fighter.p1.motion: $wait") `
    -TargetFrame 1200 -MinHits 0 -Seconds 100
