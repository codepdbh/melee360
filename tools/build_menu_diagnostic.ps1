[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$OutDir,[switch]$BootToMatch)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
if(-not $env:XEDK) { throw 'Set XEDK to the Xbox 360 SDK.' }
$build=Join-Path $root 'build-x360/xdk'
$record=Get-Content -Raw (Join-Path $root 'dist/build-info.json') | ConvertFrom-Json
if($record.boot_to_match -or $record.input_script -or $record.call_trace) {
    throw 'Build a normal live-input XEX first.'
}
if((Get-FileHash (Join-Path $root 'dist/default.xex')).Hash -ne $record.xex_sha256 -or
   (Get-FileHash (Join-Path $build 'melee360.map')).Hash -ne $record.map_sha256) {
    throw 'Normal build hashes do not match its provenance.'
}
New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path
$obj=Join-Path $OutDir 'pad-script.obj'
& "$env:XEDK/bin/win32/cl.exe" /nologo /c /O2 /MT /EHsc- /GR- /GS- /W4 /D_XBOX /DXBOX /DNDEBUG /DM360_INPUT_SCRIPT `
    "/I$env:XEDK/include/xbox" "/I$env:XEDK/include/xbox/sys" "/Fo$obj" "$root/src/xdk/melee_pad_xdk.cpp"
if($LASTEXITCODE -ne 0) { throw 'Diagnostic pad compilation failed.' }
$flow=Join-Path $build 'melee_flow_xdk.obj'
$flowObj=Join-Path $OutDir 'flow-match.obj'
if($BootToMatch) {
    & "$env:XEDK/bin/win32/cl.exe" /nologo /c /O2 /MT /EHsc- /GR- /GS- /W4 /D_XBOX /DXBOX /DNDEBUG /DM360_BOOT_TO_MATCH `
        "/I$env:XEDK/include/xbox" "/I$env:XEDK/include/xbox/sys" "/I$build" "/Fo$flowObj" "$root/src/xdk/melee_flow_xdk.cpp"
    if($LASTEXITCODE -ne 0) { throw 'Diagnostic flow compilation failed.' }
}
$rsp=Get-Content -LiteralPath (Join-Path $build 'link.rsp')
if($rsp -match '/FORCE') { throw 'Diagnostic requires a strict link response.' }
$pad=Join-Path $build 'melee_pad_xdk.obj'
if(@($rsp | Where-Object { $_ -eq ('"'+$pad+'"') }).Count -ne 1) { throw 'Expected exactly one normal pad object.' }
if($BootToMatch -and @($rsp | Where-Object { $_ -eq ('"'+$flow+'"') }).Count -ne 1) { throw 'Expected exactly one normal flow object.' }
$rsp=$rsp | ForEach-Object {
    if($_ -eq ('"'+$pad+'"')) { '"'+$obj+'"' }
    elseif($BootToMatch -and $_ -eq ('"'+$flow+'"')) { '"'+$flowObj+'"' }
    elseif($_ -match '^"/OUT:') { '"/OUT:'+ (Join-Path $OutDir 'menus.exe') +'"' }
    elseif($_ -match '^"/MAP:') { '"/MAP:'+ (Join-Path $OutDir 'menus.map') +'"' }
    elseif($_ -match '^"/PDB:') { '"/PDB:'+ (Join-Path $OutDir 'menus.pdb') +'"' }
    else { $_ }
}
$response=Join-Path $OutDir 'link.rsp'
Set-Content -Encoding ASCII -LiteralPath $response $rsp
& "$env:XEDK/bin/win32/link.exe" "@$response"
if($LASTEXITCODE -ne 0) { throw 'Diagnostic link failed.' }
$process=Start-Process -FilePath "$env:XEDK/bin/win32/imagexex.exe" -WindowStyle Hidden -Wait -PassThru `
    -ArgumentList @("/IN:`"$(Join-Path $OutDir 'menus.exe')`"", "/OUT:`"$(Join-Path $OutDir 'default.xex')`"")
if($process.ExitCode -ne 0) { throw 'Diagnostic XEX generation failed.' }
@{
    normal=$record
    diagnostic=$(if($BootToMatch){'boot-to-match flow and scripted pad'}else{'normal intro/menu entry; scripted pad only'})
    boot_to_match=[bool]$BootToMatch
    xex_sha256=(Get-FileHash (Join-Path $OutDir 'default.xex')).Hash
    map_sha256=(Get-FileHash (Join-Path $OutDir 'menus.map')).Hash
} | ConvertTo-Json -Depth 5 | Set-Content -Encoding UTF8 (Join-Path $OutDir 'build-info.json')
