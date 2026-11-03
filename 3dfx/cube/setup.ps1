# Fetch the compiler; the Glide SDK and runtimes are vendored in ../vendor.
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
    if (!(Test-Path tools/watcom/binnt64/wcl386.exe)) {
        Fetch 'https://github.com/open-watcom/open-watcom-v2/releases/download/Current-build/open-watcom-2_0-c-win-x64.exe' 'tools/watcom.exe'
        Unpack 'tools/watcom.exe' 'tools/watcom'
    }
    & ./build.ps1
} finally { Pop-Location }
