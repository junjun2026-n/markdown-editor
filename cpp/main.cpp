// ============================================================
// 文件名称：main.cpp
// 生成日期：2026-10-31
// 修改日期：2026-11-20
// 版本信息：v0.261115
// 作    者：JUN
// 备    注：本程序由AI辅助完成。
//           文件功能：组织程序入口、服务初始化及请求处理。
//           文件用途：供工具的 TOOLS/MARKDOWN/cpp/main.cpp 模块使用。
//           注意事项：运行前检查依赖与配置；涉及数据或服务变更时先备份并验证。
// ============================================================
#include "editor.h"
#include <QApplication>
#include <QFileInfo>
#include <QTimer>

int main(int argc,char **argv) {
    QApplication app(argc,argv);
    QApplication::setApplicationName("MARKDOWN C++");
    QApplication::setApplicationVersion("v0.261115");
    QApplication::setStyle("Fusion");
    EditorWindow window;
    const auto args=app.arguments();
    for(int i=1;i<args.size();++i){if(args[i]=="--smoke-test"||args[i]=="--screenshot"){if(args[i]=="--screenshot")++i;continue;}window.openPath(args[i]);}
    if(!window.tabs->count())window.newPage(QString::fromUtf8(R"(# 安静地写，清晰地表达

MARKDOWN · 原生 C++ 桌面编辑器

## :clapper: 从这里开始

直接在文档中书写。选中文字后使用 **Ctrl+B** 加粗，或按 **Ctrl+2** 设置二级标题。

- 左侧大纲帮助你在长文档中快速定位。
- **Ctrl+/** 在可视化编辑与 Markdown 源码之间切换。
- **Ctrl+S** 保存，**Ctrl+F** 查找，**Ctrl+H** 替换。
- **F8** 进入专注模式，再按一次退出。

## 让内容成为主角

> 轻量界面、舒适留白、本地文件。

| 工作方式 | 快捷键 |
| --- | --- |
| 打开文档 | Ctrl+O |
| 新建文档 | Ctrl+N |
| 切换主题 | Ctrl+Shift+D |

### 关于保存

源码模式可保留原有 Markdown 写法；可视化修改后，保存会生成标准 Markdown，部分排版写法会被规范化。
)") );
    window.show();
    const int screenshot=args.indexOf("--screenshot");
    if(screenshot>=0&&screenshot+1<args.size())QTimer::singleShot(700,&window,[&window,args,screenshot]{window.grab().save(args[screenshot+1]);QApplication::quit();});
    if(args.contains("--smoke-test"))QTimer::singleShot(1000,&app,&QApplication::quit);
    return app.exec();
}
