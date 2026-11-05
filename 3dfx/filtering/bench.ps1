$ErrorActionPreference = 'Stop'
Remove-Item -LiteralPath "$PSScriptRoot/START.MRK","$PSScriptRoot/END.MRK" -ErrorAction SilentlyContinue
$demoProcess = Start-Process (Get-Command dosbox-x.exe).Source -WorkingDirectory $PSScriptRoot -ArgumentList '-conf bench.conf' -WindowStyle Hidden -PassThru
$timer = [Diagnostics.Stopwatch]::StartNew()
while (!(Test-Path -LiteralPath "$PSScriptRoot/START.MRK")) {
    if ($demoProcess.HasExited -or $timer.Elapsed.TotalSeconds -gt 60) { throw 'Benchmark did not start' }
    Start-Sleep -Milliseconds 5
}
$timer.Restart()
while (!(Test-Path -LiteralPath "$PSScriptRoot/END.MRK")) {
    if ($demoProcess.HasExited -or $timer.Elapsed.TotalSeconds -gt 60) { throw 'Benchmark did not finish' }
    Start-Sleep -Milliseconds 5
}
$seconds = $timer.Elapsed.TotalSeconds
$wallFps = 600/$seconds
'Windows wall clock: {0:F2} FPS (600 frames / {1:F3} seconds)' -f $wallFps,$seconds
if (!$demoProcess.WaitForExit(10000)) { throw 'Benchmark did not exit' }
$result = Get-Content "$PSScriptRoot/FILTER.LOG" -Raw
$result
if ($result -notmatch '(?m)^PASS:') { throw 'Benchmark failed; see FILTER.LOG' }
if ($wallFps -lt 59.5) { throw "Host throughput below the 60 Hz target: $wallFps FPS" }
