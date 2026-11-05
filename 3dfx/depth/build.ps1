$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    $env:WATCOM = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../cube/tools/watcom'))
    $env:PATH = "$env:WATCOM\binnt64;$env:WATCOM\binnt;$env:PATH"
    $env:INCLUDE = "$env:WATCOM\h"
    & wcl386 -q -bt=dos -l=dos4g -mf -5s -fp5 -ox '-fe=DEPTH.EXE' '-i=../vendor/glide/Glide/Src/Sst1/include' '-"option nocaseexact"' '-"option stack=128k"' -d__DOS__ -d__DOS32__ depth.c ../helmet/profiler.c ../vendor/glide/Glide/Lib/Dos/Stack/glide2x.lib
    if ($LASTEXITCODE) { throw 'Compilation/link failed' }
    Copy-Item ../vendor/runtime/DOS4GW.EXE DOS4GW.EXE
    Copy-Item ../vendor/runtime/GLIDE2X.OVL GLIDE2X.OVL
} finally { Pop-Location }
