# ============================================================
# 文件名称：build-cpp.ps1
# 生成日期：2026-10-31
# 修改日期：2026-11-20
# 版本信息：v0.261115
# 作    者：JUN
# 备    注：本程序由AI辅助完成。
#           文件功能：执行 build-cpp 对应的启动、构建或运维任务。
#           文件用途：供工具的 TOOLS/MARKDOWN/build-cpp.ps1 模块使用。
#           注意事项：运行前检查依赖与配置；涉及数据或服务变更时先备份并验证。
# ============================================================
param([string]$QtRoot='C:\Qt\6.7.1\mingw_64',[string]$CompilerRoot='C:\Qt\Tools\mingw1120_64')
$ErrorActionPreference='Stop'
Set-Location $PSScriptRoot
$cmake='C:\Qt\Tools\CMake_64\bin\cmake.exe'
if(-not(Test-Path $cmake)){ $cmake=(Get-Command cmake -ErrorAction Stop).Source }
$env:PATH="$CompilerRoot\bin;$QtRoot\bin;$env:PATH"
& $cmake -S cpp -B build-cpp-mingw -G Ninja '-DCMAKE_BUILD_TYPE=Release' '-DCMAKE_MAKE_PROGRAM=C:/Qt/Tools/Ninja/ninja.exe' "-DCMAKE_CXX_COMPILER=$CompilerRoot/bin/g++.exe" "-DCMAKE_PREFIX_PATH=$QtRoot"
if($LASTEXITCODE -ne 0){throw 'CMake configure failed.'}
& $cmake --build build-cpp-mingw --parallel 4
if($LASTEXITCODE -ne 0){throw 'C++ compilation failed.'}
$env:PATH="$QtRoot\bin;$env:PATH"
$env:QT_QPA_PLATFORM_PLUGIN_PATH="$QtRoot\plugins\platforms"
& "$([IO.Path]::GetDirectoryName($cmake))\ctest.exe" --test-dir build-cpp-mingw --output-on-failure
if($LASTEXITCODE -ne 0){throw 'C++ tests failed.'}
$dest=Join-Path $PSScriptRoot 'dist-cpp'
New-Item -ItemType Directory -Path $dest -Force | Out-Null
Copy-Item build-cpp-mingw/MARKDOWN.exe $dest -Force
& "$QtRoot\bin\windeployqt.exe" --release --no-translations --no-opengl-sw --no-system-d3d-compiler "$dest\MARKDOWN.exe"
if($LASTEXITCODE -ne 0){throw 'Qt runtime deployment failed.'}
Copy-Item cpp/README.md "$dest\README.md" -Force
Copy-Item LICENSE,VERSION,COPYRIGHT.md $dest -Force
Copy-Item licenses "$dest\licenses" -Recurse -Force
$oldPluginPath=$env:QT_QPA_PLATFORM_PLUGIN_PATH
Remove-Item Env:\QT_QPA_PLATFORM_PLUGIN_PATH -ErrorAction SilentlyContinue
try {
    $smokeProcess=Start-Process -FilePath "$dest\MARKDOWN.exe" -ArgumentList '--smoke-test' -WindowStyle Hidden -Wait -PassThru
    if($smokeProcess.ExitCode -ne 0){throw 'Packaged EXE smoke test failed.'}
} finally {
    $env:QT_QPA_PLATFORM_PLUGIN_PATH=$oldPluginPath
}
Compress-Archive -Path "$dest\*" -DestinationPath MARKDOWN-Cpp-portable.zip -Force
Write-Host "Ready: $dest\MARKDOWN.exe"
