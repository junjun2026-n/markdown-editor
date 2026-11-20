# ============================================================
# 文件名称：build.ps1
# 生成日期：2026-10-31
# 修改日期：2026-11-20
# 版本信息：v0.261115
# 作    者：JUN
# 备    注：本程序由AI辅助完成。
#           文件功能：执行 build 对应的启动、构建或运维任务。
#           文件用途：供工具的 TOOLS/MARKDOWN/build.ps1 模块使用。
#           注意事项：运行前检查依赖与配置；涉及数据或服务变更时先备份并验证。
# ============================================================
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
