param([switch]$FullTour,[switch]$Stress,[switch]$ReturnPath,[switch]$Doorway)
$ErrorActionPreference = 'Stop'
$frameCount = if ($FullTour) { 6000 } elseif ($Stress -or $ReturnPath -or $Doorway) { 600 } else { 1440 }
$config = if ($FullTour) { 'tour.conf' } elseif ($Stress) { 'stress.conf' } elseif ($ReturnPath) { 'return.conf' } elseif ($Doorway) { 'door.conf' } else { 'soak.conf' }
Remove-Item -LiteralPath "$PSScriptRoot/SOAKSTAR.MRK","$PSScriptRoot/SOAKEND.MRK" -ErrorAction SilentlyContinue
Start-Process (Get-Command dosbox-x.exe).Source -WorkingDirectory $PSScriptRoot -ArgumentList "-conf $config" -WindowStyle Hidden
$timer = [Diagnostics.Stopwatch]::StartNew()
while (!(Test-Path -LiteralPath "$PSScriptRoot/SOAKSTAR.MRK")) {
    if ($timer.Elapsed.TotalSeconds -gt 60) { throw 'Journey test did not start' }
    Start-Sleep -Milliseconds 5
}
$timer.Restart()
while (!(Test-Path -LiteralPath "$PSScriptRoot/SOAKEND.MRK")) {
    if ($timer.Elapsed.TotalSeconds -gt 150) { throw 'Journey test did not finish' }
    Start-Sleep -Milliseconds 5
}
$seconds = $timer.Elapsed.TotalSeconds
$wallFps = $frameCount/$seconds
'Windows wall clock: {0:F2} FPS ({1} frames / {2:F3} seconds)' -f $wallFps,$frameCount,$seconds
$timer.Restart()
do {
    Start-Sleep -Milliseconds 100
    $result = [string](Get-Content "$PSScriptRoot/RAIN.LOG" -Raw -ErrorAction SilentlyContinue)
} while ($result -notmatch '(?m)^(PASS|FAIL):' -and $timer.Elapsed.TotalSeconds -lt 10)
$result
if ($result -notmatch '(?m)^PASS:') { throw 'Journey test failed; see RAIN.LOG' }

if ($wallFps -lt 59.5) { throw "Host throughput below the 60 Hz target: $wallFps FPS" }
