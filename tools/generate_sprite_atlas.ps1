[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string] $OutputPath
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$directory = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Path $directory -Force | Out-Null

$bitmap = [System.Drawing.Bitmap]::new(
    128, 64, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
$graphics.Clear([System.Drawing.Color]::Transparent)

function New-Brush([int] $red, [int] $green, [int] $blue, [int] $alpha = 255) {
    [System.Drawing.SolidBrush]::new(
        [System.Drawing.Color]::FromArgb($alpha, $red, $green, $blue))
}

$white = New-Brush 255 255 255
$ink = New-Brush 13 32 58
$face = New-Brush 225 242 255
$cyan = New-Brush 29 133 218
$highlight = New-Brush 86 218 255
$green = New-Brush 107 232 52
$scarf = New-Brush 255 85 137 230
$dummyFace = New-Brush 244 183 91
$dummyBody = New-Brush 169 55 76
$dummyLight = New-Brush 223 78 91
$dummyDark = New-Brush 93 42 64
$gold = New-Brush 255 199 79 240

try {
    $graphics.FillRectangle($white, 0, 0, 4, 4)

    $graphics.FillEllipse($ink, 17, 1, 30, 29)
    $graphics.FillEllipse($face, 19, 2, 26, 27)
    $graphics.FillRectangle($ink, 21, 11, 5, 4)
    $graphics.FillRectangle($ink, 37, 11, 5, 4)
    $graphics.FillRectangle($highlight, 26, 21, 12, 3)
    $graphics.FillRectangle($cyan, 17, 27, 30, 23)
    $graphics.FillRectangle($highlight, 20, 29, 24, 7)
    $graphics.FillRectangle($highlight, 10, 29, 8, 18)
    $graphics.FillRectangle($highlight, 47, 29, 8, 18)
    $graphics.FillRectangle($green, 20, 50, 10, 11)
    $graphics.FillRectangle($green, 35, 50, 10, 11)
    $graphics.FillRectangle($scarf, 12, 25, 41, 3)

    $graphics.FillEllipse($dummyDark, 78, 1, 28, 29)
    $graphics.FillEllipse($dummyFace, 80, 2, 24, 27)
    $graphics.FillRectangle($ink, 86, 11, 4, 4)
    $graphics.FillRectangle($ink, 96, 11, 4, 4)
    $graphics.FillRectangle($dummyBody, 80, 27, 24, 27)
    $graphics.FillRectangle($dummyLight, 73, 31, 7, 20)
    $graphics.FillRectangle($dummyLight, 104, 31, 7, 20)
    $graphics.FillRectangle($dummyDark, 82, 54, 9, 8)
    $graphics.FillRectangle($dummyDark, 94, 54, 9, 8)
    $graphics.FillRectangle($gold, 78, 27, 28, 4)

    $bitmap.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)
} finally {
    $graphics.Dispose()
    $bitmap.Dispose()
    $white.Dispose()
    $ink.Dispose()
    $face.Dispose()
    $cyan.Dispose()
    $highlight.Dispose()
    $green.Dispose()
    $scarf.Dispose()
    $dummyFace.Dispose()
    $dummyBody.Dispose()
    $dummyLight.Dispose()
    $dummyDark.Dispose()
    $gold.Dispose()
}

Write-Host "[M360][ASSET] created $OutputPath"
