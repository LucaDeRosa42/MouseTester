# Genera res/app.ico: mouse stilizzato con il tasto sinistro acceso, nei colori dell'app.
# Uso: powershell -ExecutionPolicy Bypass -File tools\make_icon.ps1
Add-Type -AssemblyName System.Drawing

$sizes = 16, 20, 24, 32, 40, 48, 64, 128, 256
$out = Join-Path $PSScriptRoot '..\res\app.ico'

function New-RoundRect([single]$x, [single]$y, [single]$w, [single]$h, [single]$r) {
    $p = New-Object System.Drawing.Drawing2D.GraphicsPath
    $d = 2 * $r
    $p.AddArc($x, $y, $d, $d, 180, 90)
    $p.AddArc($x + $w - $d, $y, $d, $d, 270, 90)
    $p.AddArc($x + $w - $d, $y + $h - $d, $d, $d, 0, 90)
    $p.AddArc($x, $y + $h - $d, $d, $d, 90, 90)
    $p.CloseFigure()
    return $p
}

function New-IconBitmap([int]$s) {
    $bmp = New-Object System.Drawing.Bitmap($s, $s, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = 'AntiAlias'
    $g.PixelOffsetMode = 'HighQuality'
    $g.Clear([System.Drawing.Color]::Transparent)

    $accent = [System.Drawing.Color]::FromArgb(255, 77, 194, 255)
    $body   = [System.Drawing.Color]::FromArgb(255, 34, 38, 46)
    $bg     = [System.Drawing.Color]::FromArgb(255, 17, 18, 22)

    # Sfondo: quadrato scuro arrotondato
    $bgPath = New-RoundRect 0 0 $s $s ([single]($s * 0.22))
    $g.FillPath((New-Object System.Drawing.SolidBrush($bg)), $bgPath)

    # Corpo del mouse: capsula verticale
    $mw = $s * 0.50; $mh = $s * 0.74
    $mx = ($s - $mw) / 2; $my = ($s - $mh) / 2
    $mouse = New-RoundRect $mx $my $mw $mh ([single]($mw * 0.5))
    $g.FillPath((New-Object System.Drawing.SolidBrush($body)), $mouse)

    # Tasto sinistro acceso: metà superiore sinistra del corpo
    $split = $my + $mh * 0.42
    $g.SetClip($mouse)
    $g.FillRectangle((New-Object System.Drawing.SolidBrush($accent)), [single]$mx, [single]$my, [single]($mw / 2), [single]($split - $my))
    $g.ResetClip()

    # Contorno, divisione dei tasti e rotella
    $penW = [single][Math]::Max(1.0, $s * 0.045)
    $pen = New-Object System.Drawing.Pen($accent, $penW)
    $g.DrawPath($pen, $mouse)
    $thin = New-Object System.Drawing.Pen($accent, [single][Math]::Max(1.0, $s * 0.03))
    $g.DrawLine($thin, [single]$mx, [single]$split, [single]($mx + $mw), [single]$split)
    $g.DrawLine($thin, [single]($mx + $mw / 2), [single]$my, [single]($mx + $mw / 2), [single]$split)
    if ($s -ge 24) {
        $ww = $mw * 0.16; $wh = $mh * 0.16
        $wheel = New-RoundRect ([single]($mx + ($mw - $ww) / 2)) ([single]($my + $mh * 0.14)) $ww $wh ([single]($ww / 2))
        $g.FillPath((New-Object System.Drawing.SolidBrush($body)), $wheel)
        $g.DrawPath($thin, $wheel)
    }
    $g.Dispose()
    return $bmp
}

# File .ico con immagini PNG (supportate da Windows Vista in poi)
$pngs = foreach ($s in $sizes) {
    $bmp = New-IconBitmap $s
    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    , $ms.ToArray()
}

New-Item -ItemType Directory -Force (Split-Path $out) | Out-Null
$fs = [System.IO.File]::Create($out)
$w = New-Object System.IO.BinaryWriter($fs)
$w.Write([UInt16]0); $w.Write([UInt16]1); $w.Write([UInt16]$sizes.Count)
$offset = 6 + 16 * $sizes.Count
for ($i = 0; $i -lt $sizes.Count; $i++) {
    $s = $sizes[$i]
    $dim = if ($s -ge 256) { 0 } else { $s }
    $w.Write([byte]$dim); $w.Write([byte]$dim); $w.Write([byte]0); $w.Write([byte]0)
    $w.Write([UInt16]1); $w.Write([UInt16]32)
    $w.Write([UInt32]$pngs[$i].Length); $w.Write([UInt32]$offset)
    $offset += $pngs[$i].Length
}
foreach ($p in $pngs) { $w.Write($p) }
$w.Close()

# Anteprima a 256 px per controllare il disegno
$preview = New-IconBitmap 256
$preview.Save((Join-Path (Split-Path $out) 'app_preview.png'), [System.Drawing.Imaging.ImageFormat]::Png)
$preview.Dispose()
Write-Output "Creato $out"
