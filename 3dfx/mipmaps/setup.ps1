$ErrorActionPreference = 'Stop'
if (!(Test-Path "$PSScriptRoot/../cube/tools/watcom/binnt64/wcl386.exe")) {
    & "$PSScriptRoot/../cube/setup.ps1"
}
& "$PSScriptRoot/build.ps1"
