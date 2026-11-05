$ErrorActionPreference = 'Stop'
Start-Process (Get-Command dosbox-x.exe).Source -WorkingDirectory $PSScriptRoot -ArgumentList '-conf dosbox-x.conf' -WindowStyle Normal
