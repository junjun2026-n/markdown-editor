// ============================================================
// 文件名称：document.h
// 生成日期：2026-10-31
// 修改日期：2026-11-20
// 版本信息：v0.261115
// 作    者：JUN
// 备    注：本程序由AI辅助完成。
//           文件功能：实现 document 模块相关功能。
//           文件用途：供工具的 TOOLS/MARKDOWN/cpp/document.h 模块使用。
//           注意事项：运行前检查依赖与配置；涉及数据或服务变更时先备份并验证。
// ============================================================
#pragma once
#include <QString>
#include <QByteArray>

struct FileDocument {
    QString path;
    QString text;
    QByteArray encoding = "UTF-8";
    QString newline = "\n";
    QByteArray snapshot;
    bool bom = false;
};
bool readDocument(const QString &path, FileDocument &result, QString &error);
bool writeDocument(const FileDocument &document, const QString &path, QByteArray &written, QString &error);
bool isMarkdown(const QString &path);
