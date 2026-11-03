$ErrorActionPreference = 'Stop'
# Reuse the existing local compiler; the cube setup downloads it if missing.
if (!(Test-Path "$PSScriptRoot/../cube/tools/watcom/binnt64/wcl386.exe")) {
    & "$PSScriptRoot/../cube/setup.ps1"
}
& "$PSScriptRoot/build.ps1"
