$ErrorActionPreference = 'Stop'
& "$PSScriptRoot/build.ps1"
$emulator = (Get-Command dosbox-x.exe).Source
foreach ($config in 'test.conf','software.conf') {
    Remove-Item -LiteralPath "$PSScriptRoot/REFLECT.LOG" -ErrorAction SilentlyContinue
    $demoProcess = Start-Process $emulator -WorkingDirectory $PSScriptRoot -ArgumentList "-conf $config" -WindowStyle Hidden -PassThru
    $deadline = [Diagnostics.Stopwatch]::StartNew()
    while (!$demoProcess.HasExited) {
        if ($deadline.Elapsed.TotalSeconds -gt 150) { $demoProcess.Kill(); throw "$config timed out" }
        Start-Sleep -Milliseconds 100
    }
    $result = Get-Content "$PSScriptRoot/REFLECT.LOG" -Raw
    Copy-Item "$PSScriptRoot/REFLECT.LOG" "$PSScriptRoot/$($config.Replace('.conf','.LOG'))"
    "$config`n$result"
    if ($result -notmatch '(?m)^PASS:') { throw "$config failed; see REFLECT.LOG" }
}
