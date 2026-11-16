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
