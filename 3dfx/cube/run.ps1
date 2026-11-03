$ErrorActionPreference = 'Stop'
Start-Process -FilePath (Get-Command dosbox-x.exe).Source -WorkingDirectory $PSScriptRoot -ArgumentList '-conf dosbox-x.conf' -WindowStyle Normal
