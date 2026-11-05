$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    & python make-assets.py
    if ($LASTEXITCODE) { throw 'Fixture generation failed' }
    $texus = '../vendor/glide/Glide/Bin/Win32/texus.exe'
    foreach ($image in 'A','B') {
        foreach ($conversion in @(@('rgb565','565'),@('p8','P8'),@('yiq','YIQ'))) {
            $format = $conversion[0]
            $output = "assets/$image$($conversion[1]).3df"
            $staging = 'assets/build.3df'
            Remove-Item -LiteralPath $staging -ErrorAction SilentlyContinue
            $conversionProcess = Start-Process $texus -ArgumentList @('-rn','-mn','-dn','-ys','-t',$format,'-o',$staging,"assets/$image.TGA") -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput "assets/$image$($conversion[1]).LOG" -RedirectStandardError "assets/$image$($conversion[1])-error.LOG"
            if ($conversionProcess.ExitCode -or !(Test-Path -LiteralPath $staging)) { throw "TexUS $format failed; see assets logs" }
            Move-Item -LiteralPath $staging -Destination $output -Force
        }
    }
    'Generated two original images and six single-level TexUS textures.'
} finally { Pop-Location }
