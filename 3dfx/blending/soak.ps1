$ErrorActionPreference = 'Stop'
Remove-Item -LiteralPath "$PSScriptRoot/SOAKSTAR.MRK","$PSScriptRoot/SOAKEND.MRK" -ErrorAction SilentlyContinue
Start-Process (Get-Command dosbox-x.exe).Source -WorkingDirectory $PSScriptRoot -ArgumentList '-conf soak.conf' -WindowStyle Hidden
$timer = [Diagnostics.Stopwatch]::StartNew()
while (!(Test-Path -LiteralPath "$PSScriptRoot/SOAKSTAR.MRK")) {
    if ($timer.Elapsed.TotalSeconds -gt 60) { throw 'Rotation test did not start' }
    Start-Sleep -Milliseconds 5
}
$timer.Restart()
while (!(Test-Path -LiteralPath "$PSScriptRoot/SOAKEND.MRK")) {
    if ($timer.Elapsed.TotalSeconds -gt 90) { throw 'Rotation test did not finish' }
    Start-Sleep -Milliseconds 5
}
$seconds = $timer.Elapsed.TotalSeconds
'Windows wall clock: {0:F2} FPS (1440 frames / {1:F3} seconds)' -f (1440/$seconds),$seconds
$timer.Restart()
do {
    Start-Sleep -Milliseconds 100
    $result = [string](Get-Content "$PSScriptRoot/BLENDING.LOG" -Raw -ErrorAction SilentlyContinue)
} while ($result -notmatch '(?m)^(PASS|FAIL):' -and $timer.Elapsed.TotalSeconds -lt 10)
$result
if ($result -notmatch '(?m)^PASS:') { throw 'Rotation test failed; see BLENDING.LOG' }
