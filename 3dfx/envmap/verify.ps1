$ErrorActionPreference='Stop'
& "$PSScriptRoot/build.ps1"
foreach($config in 'profile.conf','detail.conf','lod.conf','software.conf') {
    Remove-Item -LiteralPath "$PSScriptRoot/ENVMAP.LOG" -ErrorAction SilentlyContinue
    $demo=Start-Process (Get-Command dosbox-x.exe).Source -WorkingDirectory $PSScriptRoot -ArgumentList "-conf $config" -WindowStyle Hidden -PassThru
    $timer=[Diagnostics.Stopwatch]::StartNew()
    while(!$demo.HasExited) {
        if($timer.Elapsed.TotalSeconds -gt 180){$demo.Kill();throw "$config timed out"}
        Start-Sleep -Milliseconds 100
    }
    $result=Get-Content "$PSScriptRoot/ENVMAP.LOG" -Raw
    Copy-Item "$PSScriptRoot/ENVMAP.LOG" "$PSScriptRoot/$($config.Replace('.conf','.LOG'))"
    "$config`n$result"
    if($result -notmatch '(?m)^PASS:'){throw "$config failed; see ENVMAP.LOG"}
}
