$ErrorActionPreference = 'Stop'
$demoDir = $PSScriptRoot
Remove-Item -LiteralPath "$demoDir/START.MRK","$demoDir/END.MRK" -ErrorAction SilentlyContinue
Start-Process (Get-Command dosbox-x.exe).Source -WorkingDirectory $demoDir -ArgumentList '-conf bench.conf' -WindowStyle Normal
$timer = [Diagnostics.Stopwatch]::StartNew()
while (!(Test-Path "$demoDir/START.MRK")) {
    if ($timer.Elapsed.TotalSeconds -gt 20) { throw 'Benchmark did not start' }
    Start-Sleep -Milliseconds 2
}
$timer.Restart()
while (!(Test-Path "$demoDir/END.MRK")) {
    if ($timer.Elapsed.TotalSeconds -gt 60) { throw 'Benchmark did not finish' }
    Start-Sleep -Milliseconds 2
}
$seconds = $timer.Elapsed.TotalSeconds
'{0:F0} refresh-paced FPS (600 frames in {1:F3}s; Windows wall clock)' -f (600/$seconds),$seconds
# Let the framebuffer capture and log finish before another run can start.
$timer.Restart()
while (!((Get-Content "$demoDir/CUBE.LOG" -Raw -ErrorAction SilentlyContinue) -match '(PASS|FAIL):')) {
    if ($timer.Elapsed.TotalSeconds -gt 15) { throw 'Benchmark capture did not finish' }
    Start-Sleep -Milliseconds 100
}
