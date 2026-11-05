$ErrorActionPreference = 'Stop'
& "$PSScriptRoot/build.ps1"
$emulator = (Get-Command dosbox-x.exe).Source
foreach ($config in 'test.conf','software.conf') {
    Remove-Item -LiteralPath "$PSScriptRoot/PAINT.LOG" -ErrorAction SilentlyContinue
    $demoProcess = Start-Process $emulator -WorkingDirectory $PSScriptRoot -ArgumentList "-conf $config" -WindowStyle Hidden -PassThru
    $deadline = [Diagnostics.Stopwatch]::StartNew()
    while (!$demoProcess.HasExited) {
        if ($deadline.Elapsed.TotalSeconds -gt 90) { $demoProcess.Kill(); throw "$config timed out" }
        Start-Sleep -Milliseconds 100
    }
    $result = Get-Content "$PSScriptRoot/PAINT.LOG" -Raw
    Copy-Item "$PSScriptRoot/PAINT.LOG" "$PSScriptRoot/$($config.Replace('.conf','.LOG'))"
    "$config`n$result"
    if ($result -notmatch '(?m)^PASS:') { throw "$config failed; see PAINT.LOG" }
}
