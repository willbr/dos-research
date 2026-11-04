$ErrorActionPreference = 'Stop'
& "$PSScriptRoot/build.ps1"
$emulator = (Get-Command dosbox-x.exe).Source
foreach ($config in 'test.conf','software.conf') {
    Remove-Item -LiteralPath "$PSScriptRoot/LIGHTMAP.LOG" -ErrorAction SilentlyContinue
    $demoProcess = Start-Process $emulator -WorkingDirectory $PSScriptRoot -ArgumentList "-conf $config" -WindowStyle Hidden -PassThru
    if (!$demoProcess.WaitForExit(60000)) {
        $demoProcess.Kill()
        throw "$config timed out"
    }
    $result = Get-Content "$PSScriptRoot/LIGHTMAP.LOG" -Raw
    Copy-Item "$PSScriptRoot/LIGHTMAP.LOG" "$PSScriptRoot/$($config.Replace('.conf','.LOG'))"
    "$config`n$result"
    if ($result -notmatch '(?m)^PASS:') { throw "$config failed; see LIGHTMAP.LOG" }
}
