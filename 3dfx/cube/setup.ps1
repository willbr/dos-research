# Fetch the original toolchain and runtime into this demo's local directory.
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Force tools | Out-Null
    function Fetch($Url, $Destination) {
        & curl.exe -L --fail --output $Destination $Url
        if ($LASTEXITCODE) { throw "Download failed: $Url" }
    }
    function Unpack($Archive, $Destination) {
        & 7z.exe x $Archive "-o$Destination" -y | Out-Null
        if ($LASTEXITCODE -gt 1) { throw "Extraction failed: $Archive" }
    }
    Fetch 'https://github.com/open-watcom/open-watcom-v2/releases/download/Current-build/open-watcom-2_0-c-win-x64.exe' 'tools/watcom.exe'
    Unpack 'tools/watcom.exe' 'tools/watcom'
    Fetch 'https://3dfxarchive.com/downloads/glide_sdk-243.zip' 'tools/sdk.zip'
    Unpack 'tools/sdk.zip' 'tools/sdk'
    foreach ($part in @('gwebvgr.exe','gwebvgr2.exe','gwebvgr3.exe')) {
        Unpack "tools/sdk/$part" 'tools/glide'
    }
    Fetch 'https://raw.githubusercontent.com/efliks/nostalgic-glide-demo/master/glide2x.ovl' 'GLIDE2X.OVL'
    & ./build.ps1
} finally { Pop-Location }
