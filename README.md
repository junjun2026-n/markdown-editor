# MARKDOWN 桌面编辑器

???**v0.261115** ? ???????? ? [GPLv3 ??](LICENSE) ? [????](COPYRIGHT.md)

## C++ 新版

已提供原生 C++ / Qt 重写版，采用 Typora 风格的单栏可视化编辑、文档大纲、专注模式和深浅色主题。

- 启动：`dist-cpp\MARKDOWN.exe`（请保留同目录 DLL 和插件）。
- 便携包：`MARKDOWN-Cpp-portable.zip`（完整解压后运行）。
- 源码：`cpp`；构建：`build-cpp.ps1`；[新版详细说明](cpp/README.md)。
- `dist\MARKDOWN.exe` 为保留的 Python 旧版。

下面说明适用于 Python 旧版。

基于 Python 3.10+、PySide6 / Qt 的 Windows 本地桌面工具，可编辑和显示 Markdown 与文本文件。无需浏览器或服务器。

## 直接使用 EXE

双击 `dist\MARKDOWN.exe`。可将该 EXE 单独复制到其他 Windows 64 位电脑，无需安装 Python。单文件程序首次启动需要解压 Qt 运行组件，可能等待数秒。

`dist\MARKDOWN-portable.zip` 包含 EXE、使用说明和依赖许可文本，可用于复制分发，解压后双击 EXE 即可。

也可将文件拖到 EXE 图标上，或使用命令行：

```powershell
.\dist\MARKDOWN.exe "D:\文档\说明.md" "D:\文档\日志.txt"
```

## 功能和操作

- 左侧编辑、右侧预览，约 220 毫秒延迟更新；工具栏可切换仅编辑、仅预览。
- Markdown 支持标题、粗斜体、列表、引用、代码块、表格、链接和本地图片。相对图片路径基于当前文档目录解析。
- 兼容 `&#x20;[Slides]\(./00Summary/)` 等导出文本中的转义链接，预览显示可点击的 Slides；HTML 空格实体自动解析。相对目录链接优先在编辑器打开目录内的 README.md 或 index.md，没有说明文件时打开系统文件夹。代码示例和源文件保持原样。
- 支持常用 Emoji 短代码，例如 `## :clapper: 课程内容大纲` 预览为“🎬 课程内容大纲”。仅转换预览和导出的 HTML，源文件、代码块、行内代码及链接地址保持原文；未知短代码保持原样。
- 文本文件按纯文本显示，不将 `#` 等符号解释为 Markdown。
- 多页签、多选打开文件；支持将文件拖到窗口空白区域。相同文件不会重复打开。
- 新建 `Ctrl+N`、打开 `Ctrl+O`、保存 `Ctrl+S`、另存为 `Ctrl+Shift+S`、关闭页签 `Ctrl+W`。
- 查找 `Ctrl+F`、替换 `Ctrl+H`，支持区分大小写、向前/向后循环查找和全部替换。
- 撤销 `Ctrl+Z`、重做 `Ctrl+Y`，全部替换可一次撤销。
- 视图菜单可放大/缩小；状态栏显示文件路径、编码、换行格式、行列及字符数。
- 文件菜单可导出 HTML；图片引用保留为引用，不嵌入导出文件。
- 未保存文档关闭时提供保存、放弃、取消；磁盘文件被其他程序修改后，保存前提示是否覆盖。
- 新建文件使用 UTF-8；读取支持 UTF-8（含 BOM）、UTF-16 BOM、GB18030（兼容常见 GBK 文件），保存保留原有编码和换行。无 BOM 且无法按 UTF-8 解码的文件按 GB18030 尝试，不覆盖无法解码的原文件。
- 保存采用同目录临时文件写完再替换，降低写入失败损坏原文件的风险。

预览采用 Qt 内置 Markdown 渲染，不运行 JavaScript，不支持 Mermaid、数学公式或网页级 HTML/CSS；代码块显示为等宽文本，不提供语法高亮。外部 HTTP/HTTPS 链接点击后用默认浏览器打开。超大日志文件建议使用专用查看工具。

## 源码开发

```powershell
cd C:\20260501\qionghai\TOOLS\MARKDOWN
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements-build.txt
.\.venv\Scripts\python.exe main.py
```

也可执行 `start.ps1`，使用本项目虚拟环境启动，不影响其他项目。

## 测试及重新打包

```powershell
.\.venv\Scripts\python.exe -m unittest discover -s tests -v
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

产物为 `dist\MARKDOWN.exe`，PyInstaller 配置自动保存在 `MARKDOWN.spec`。源码位于 `main.py`（界面）和 `documents.py`（编码及文件保存）。构建目录 `build`、虚拟环境 `.venv` 均可重新生成。

Qt / PySide6 组件采用其各自的开源许可，相关许可文本随工具源码放在 `licenses` 目录；对外再分发时请保留并遵守对应许可。
