$ErrorActionPreference = 'Stop'
$demoDir = $PSScriptRoot
$labels = @('Textures + highlights / AA off', 'Textures + highlights / AA on', 'Textures only / AA off')
foreach ($phase in 0..2) {
    Remove-Item -LiteralPath "$demoDir/P${phase}START.MRK","$demoDir/P${phase}END.MRK" -ErrorAction SilentlyContinue
}
Start-Process (Get-Command dosbox-x.exe).Source -WorkingDirectory $demoDir -ArgumentList '-conf bench.conf' -WindowStyle Normal
foreach ($phase in 0..2) {
    $timer = [Diagnostics.Stopwatch]::StartNew()
    while (!(Test-Path -LiteralPath "$demoDir/P${phase}START.MRK")) {
        if ($timer.Elapsed.TotalSeconds -gt 60) { throw "Phase $phase did not start" }
        Start-Sleep -Milliseconds 5
    }
    $timer.Restart()
    while (!(Test-Path -LiteralPath "$demoDir/P${phase}END.MRK")) {
        if ($timer.Elapsed.TotalSeconds -gt 60) { throw "Phase $phase did not finish" }
        Start-Sleep -Milliseconds 5
    }
    $seconds = $timer.Elapsed.TotalSeconds
    '{0}: {1:F1} FPS (60 frames / {2:F3} seconds)' -f $labels[$phase], (60/$seconds), $seconds
}
