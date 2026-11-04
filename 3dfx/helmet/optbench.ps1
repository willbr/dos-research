param([switch]$ResultsOnly)
$ErrorActionPreference = 'Stop'
$demoDir = $PSScriptRoot
$logPath = Join-Path $demoDir 'OPTBENCH.LOG'
if (!$ResultsOnly) {
    $exe = (Get-Command dosbox-x.exe).Source
    Remove-Item -LiteralPath $logPath -ErrorAction SilentlyContinue
    $process = Start-Process $exe -WorkingDirectory $demoDir -ArgumentList '-conf optbench.conf' -WindowStyle Hidden -PassThru
    try {
        if (!$process.WaitForExit(300000)) { throw 'Optimization benchmark exceeded five minutes' }
    } finally {
        $process.Dispose()
    }
}
$result = Get-Content -LiteralPath $logPath -Raw
$result
if ($result -notmatch '(?m)^PASS:') { throw 'Optimization accuracy check failed; see OPTBENCH.LOG' }
$culture = [Globalization.CultureInfo]::InvariantCulture
$quality = ''
$rows = foreach ($line in ($result -split '\r?\n')) {
    if ($line -match '^QUALITY (\w+):') { $quality = $Matches[1] }
    if ($line -match '^(.*?) \| prepare ms (\S+) (\S+) (\S+) \| frame (\S+) work (\S+) transform (\S+) \| error xy (\S+) rgb (\S+) spec (\S+) uv (\S+) topology (\S+)$') {
        $mean = ([double]::Parse($Matches[2],$culture)+[double]::Parse($Matches[3],$culture)+[double]::Parse($Matches[4],$culture))/3
        [pscustomobject][ordered]@{
            quality=$quality; variant=$Matches[1]
            prepare_trial1_ms=$Matches[2]; prepare_trial2_ms=$Matches[3]; prepare_trial3_ms=$Matches[4]
            prepare_mean_ms=$mean.ToString('F6',$culture)
            frame_ms=$Matches[5]; work_ms=$Matches[6]; render_transform_ms=$Matches[7]
            max_xy_pixels=$Matches[8]; max_rgb=$Matches[9]; max_spec=$Matches[10]
            max_uv=$Matches[11]; topology_mismatches=$Matches[12]
        }
    }
}
if (@($rows).Count -ne 20) { throw 'Expected ten variants for each of two quality settings' }
$rows | Export-Csv -LiteralPath (Join-Path $demoDir 'benchmark-results.csv') -NoTypeInformation -Encoding utf8
