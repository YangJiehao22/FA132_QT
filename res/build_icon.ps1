# Build DtSample.ico from FA132_Qtech_app_icon.png (Qtech brand, default).
param(
    [string]$SourcePng = "",
    [string]$Root = ""
)

Add-Type -AssemblyName System.Drawing

if ([string]::IsNullOrEmpty($Root)) {
    $Root = Split-Path -Parent $MyInvocation.MyCommand.Path
}
if ([string]::IsNullOrEmpty($SourcePng)) {
    $SourcePng = Join-Path $Root "FA132_Qtech_app_icon.png"
}

if (-not (Test-Path $SourcePng)) {
    Write-Error "Source PNG not found: $SourcePng"
    exit 1
}

$icoPath = Join-Path $Root "DtSample.ico"
$squarePath = Join-Path $Root "FA132_Qtech_app_icon_square.png"

$src = [System.Drawing.Bitmap]::FromFile($SourcePng)
# Center square crop (no stretch) — keeps Qtech logo proportions from source art.
$side = [Math]::Min($src.Width, $src.Height)
$cropX = [int](($src.Width - $side) / 2)
$cropY = [int](($src.Height - $side) / 2)

$canvas = 256
$sq = New-Object System.Drawing.Bitmap $canvas, $canvas
$g = [System.Drawing.Graphics]::FromImage($sq)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$srcRect = New-Object System.Drawing.Rectangle $cropX, $cropY, $side, $side
$dstRect = New-Object System.Drawing.Rectangle 0, 0, $canvas, $canvas
$g.DrawImage($src, $dstRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
$g.Dispose()
$src.Dispose()

$sq.Save($squarePath, [System.Drawing.Imaging.ImageFormat]::Png)

function Get-PngBytes([System.Drawing.Bitmap]$master, [int]$size) {
    $bmp = New-Object System.Drawing.Bitmap $size, $size
    $g2 = [System.Drawing.Graphics]::FromImage($bmp)
    $g2.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g2.DrawImage($master, 0, 0, $size, $size)
    $g2.Dispose()
    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    $bytes = $ms.ToArray()
    $ms.Dispose()
    return $bytes
}

$sizes = @(16, 24, 32, 48, 64, 128, 256)
$pngs = New-Object System.Collections.Generic.List[byte[]]
foreach ($s in $sizes) { [void]$pngs.Add((Get-PngBytes $sq $s)) }

$fs = [System.IO.File]::Create($icoPath)
$bw = New-Object System.IO.BinaryWriter $fs
$bw.Write([uint16]0)
$bw.Write([uint16]1)
$bw.Write([uint16]$sizes.Count)
$offset = 6 + 16 * $sizes.Count
foreach ($i in 0..($sizes.Count - 1)) {
    $s = $sizes[$i]
    $w = [byte]($(if ($s -ge 256) { 0 } else { $s }))
    $h = [byte]($(if ($s -ge 256) { 0 } else { $s }))
    $bw.Write($w)
    $bw.Write($h)
    $bw.Write([byte]0)
    $bw.Write([byte]0)
    $bw.Write([uint16]1)
    $bw.Write([uint16]32)
    $bw.Write([uint32]$pngs[$i].Length)
    $bw.Write([uint32]$offset)
    $offset += $pngs[$i].Length
}
foreach ($p in $pngs) { $bw.Write($p) }
$bw.Close()
$fs.Close()
$sq.Dispose()

Write-Host "Source: $SourcePng (Qtech, crop ${side}x${side})"
Write-Host "Preview: $squarePath"
Write-Host "ICO: $icoPath"
