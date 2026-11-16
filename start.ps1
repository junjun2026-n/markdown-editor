$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
& "$PSScriptRoot\.venv\Scripts\pythonw.exe" "$PSScriptRoot\main.py" @args
