$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    $env:WATCOM = Join-Path $PSScriptRoot 'tools/watcom'
    $env:PATH = "$env:WATCOM\binnt64;$env:WATCOM\binnt;$env:PATH"
    $env:INCLUDE = "$env:WATCOM\h"
    & wcl386 -q -bt=dos -l=dos4g -mf -5s -fp5 -ox '-fe=CUBE.EXE' '-i=tools/glide/Glide/Src/Sst1/include' '-"option nocaseexact"' -dGLIDE_BUILD -d__DOS__ -d__DOS32__ cube.c tools/glide/Glide/Lib/Dos/Stack/glide2x.lib
    if ($LASTEXITCODE) { throw 'Compilation failed' }
    Copy-Item "$env:WATCOM\binw\dos4gw.exe" DOS4GW.EXE
} finally { Pop-Location }
