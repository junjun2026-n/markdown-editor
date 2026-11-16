# MARKDOWN C++ 2.0

原生 C++17 / Qt 6.7 桌面 Markdown、文本编辑器，采用 Typora 风格的简洁单栏、留白排版和左侧文档大纲。本项目是独立实现，并非 Typora 官方产品。

## 运行

双击本目录的 `MARKDOWN.exe`。不需要 Python、网络或后台服务。

**请保留 EXE 同目录的 DLL、platforms、styles 等文件夹。** 可将整个目录复制到其他 Windows 64 位电脑；分发请使用 `MARKDOWN-Cpp-portable.zip`，先完整解压再运行。C++ 程序采用 Qt 动态链接，并非单个 EXE 即可移走的打包方式。

## 编辑方式

默认是单栏可视化编辑：直接在排版后的文档中输入内容，支持多页签、大纲定位、标题、粗斜体、列表和表格。使用“段落”菜单或快捷键设置格式。

- `Ctrl+/`：切换 Markdown 源码与可视化编辑。
- `Ctrl+1 / 2 / 3`：一级、二级、三级标题；`Ctrl+0`：正文。
- `Ctrl+B / Ctrl+I`：加粗 / 斜体。
- `Ctrl+N / O / S`：新建 / 打开 / 保存；`Ctrl+Shift+S`：另存为。
- `Ctrl+F / Ctrl+H`：查找 / 替换，支持区分大小写、循环查找及全部替换。
- `Ctrl+Z / Ctrl+Y`：撤销 / 重做。
- `Ctrl+Shift+L`：显示或隐藏大纲。
- `F8`：专注模式，隐藏菜单、工具栏、大纲、状态栏，再按一次退出。
- `Ctrl+Shift+D`：切换深色 / 浅色主题。
- `Ctrl++ / Ctrl+-`：缩放。
- 单击或右键“打开链接”：打开文档链接；拖动链接文字仍可选择编辑，Ctrl+单击同样支持。

源码模式保留 Markdown 原文，切回可视化时更新显示。**可视化修改后，保存会生成标准 Markdown，因此空行、表格、强调符号等写法可能被规范化，原有 Emoji 短代码会成为 Unicode 表情。** 需要保留精确写法的文档请在源码模式编辑。没有编辑时打开、切换模式、保存，不会将源文件转换为其他 Markdown 写法。

可视化模式在行首输入 `#` 至 `######`、`-`、`*`、`1.` 或 `>` 再按空格，会转换为标题、列表或引用；标题末尾回车恢复正文。其他复杂语法可使用段落菜单或源码模式输入后切回。复杂 HTML、Mermaid、公式、脚注和插件不在当前版本支持范围内。代码块为等宽排版，不提供语法高亮。

## 兼容与文件安全

- 延续 `:clapper:` 等 5316 个表情名称/别名映射，代码块、行内代码不转换。
- 兼容 `&#x20;[Slides]\(./00Summary/)` 等转义链接；目录链接优先打开 README.md 或 index.md。
- 本地链接以当前文档目录为起点，支持相对路径、上级目录、中文/空格路径、Windows 绝对路径和 file URL。PDF、HTML、图片等交由系统默认程序打开；无 README 的目录打开资源管理器，目标不存在时显示完整路径提示。
- `.txt`、日志及其他文本文件保持纯文本语义。
- 支持 UTF-8（含 BOM）、UTF-16LE/BE BOM、GB18030；保存保留读取的编码和换行格式。可在“文件 → 保存编码”中改为 UTF-8，以保存原编码不能表示的表情。
- 使用 QSaveFile 原子写入，外部更改检测，未保存文档关闭确认；相同文件不会重复打开。
- 文件菜单支持导出 HTML，图片保持相对引用。

## 源码构建

源码位于 `cpp`，使用 `CMakeLists.txt` 构建；没有 Python 运行依赖。表情名称表 `emoji.json` 已包含在 Qt 资源中。

本机已安装 Qt 6.7.1、MinGW 11.2、CMake、Ninja：

```powershell
cd C:\20260501\qionghai\TOOLS\MARKDOWN
powershell -ExecutionPolicy Bypass -File .\build-cpp.ps1
```

构建脚本编译 C++、运行 Qt Test、调用 windeployqt 复制运行依赖并生成便携 ZIP。依赖位置可以通过 `-QtRoot` 和 `-CompilerRoot` 参数指定。构建输出 `build-cpp-mingw`，可运行目录 `dist-cpp`。

Python 版源码及虚拟环境继续保留；C++ 版无需它们即可独立运行。Qt、MinGW 及 Emoji 数据具有各自开源许可，详见 licenses，分发时请保留许可文件。
