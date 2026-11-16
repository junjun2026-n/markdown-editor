$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
if (-not (Test-Path '.venv\Scripts\python.exe')) {
    python -m venv .venv
    if ($LASTEXITCODE -ne 0) { throw 'Virtual environment creation failed.' }
}
& .\.venv\Scripts\python.exe -m pip install -r requirements-build.txt
if ($LASTEXITCODE -ne 0) { throw 'Dependency installation failed.' }
& .\.venv\Scripts\python.exe -m unittest discover -s tests -v
if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
& .\.venv\Scripts\python.exe -m PyInstaller --noconfirm --clean --onefile --windowed --collect-data emoji --name MARKDOWN main.py
if ($LASTEXITCODE -ne 0) { throw 'EXE build failed.' }
Copy-Item -LiteralPath README.md -Destination dist/README.md -Force
Copy-Item LICENSE,VERSION,COPYRIGHT.md dist -Force
Copy-Item -LiteralPath licenses -Destination dist -Recurse -Force
Compress-Archive -Path dist/MARKDOWN.exe,dist/README.md,dist/LICENSE,dist/VERSION,dist/COPYRIGHT.md,dist/licenses -DestinationPath dist/MARKDOWN-portable.zip -Force
Write-Host "EXE: $PSScriptRoot\dist\MARKDOWN.exe"
