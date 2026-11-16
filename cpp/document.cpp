#include "document.h"
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringConverter>
#include <QTextCodec>

bool isMarkdown(const QString &path) {
    const auto ext = QFileInfo(path).suffix().toLower();
    return path.isEmpty() || ext == "md" || ext == "markdown" || ext == "mdown";
}

bool readDocument(const QString &path, FileDocument &result, QString &error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { error = file.errorString(); return false; }
    const auto raw = file.readAll();
    FileDocument value;
    value.path = QFileInfo(path).absoluteFilePath();
    value.snapshot = raw;
    QByteArray data = raw;
    if (data.startsWith("\xEF\xBB\xBF")) { value.bom = true; data.remove(0, 3); }
    else if (data.startsWith("\xFF\xFE")) { value.encoding = "UTF-16LE"; value.bom = true; data.remove(0, 2); }
    else if (data.startsWith("\xFE\xFF")) { value.encoding = "UTF-16BE"; value.bom = true; data.remove(0, 2); }
    auto codec = QTextCodec::codecForName(value.encoding);
    QTextCodec::ConverterState state(QTextCodec::IgnoreHeader);
    value.text = codec->toUnicode(data.constData(), data.size(), &state);
    if (state.invalidChars && value.encoding == "UTF-8" && !value.bom) {
        value.encoding = "GB18030";
        codec = QTextCodec::codecForName(value.encoding);
        QTextCodec::ConverterState gbState(QTextCodec::IgnoreHeader);
        value.text = codec->toUnicode(data.constData(), data.size(), &gbState);
        if (gbState.invalidChars || gbState.remainingChars) { error = "无法识别文件编码。"; return false; }
    } else if (state.invalidChars || state.remainingChars) { error = "文件编码损坏或内容不完整。"; return false; }
    if (value.text.contains(QChar(0))) { error = "该文件包含二进制内容，不能作为文本编辑。"; return false; }
    value.newline = value.text.contains("\r\n") ? "\r\n" : (value.text.contains('\r') ? "\r" : "\n");
    value.text.replace("\r\n", "\n").replace('\r', '\n');
    result = value;
    return true;
}

bool writeDocument(const FileDocument &document, const QString &path, QByteArray &written, QString &error) {
    auto text = document.text;
    text.replace("\n", document.newline);
    auto codec = QTextCodec::codecForName(document.encoding);
    if (!codec) { error = "不支持的文件编码。"; return false; }
    QTextCodec::ConverterState state(QTextCodec::IgnoreHeader);
    auto data = codec->fromUnicode(text.constData(), text.size(), &state);
    if (state.invalidChars) { error = "当前编码无法保存新增字符，请将编码切换为 UTF-8。"; return false; }
    if (document.bom) {
        if (document.encoding == "UTF-8") data.prepend(QByteArray::fromHex("efbbbf"));
        if (document.encoding == "UTF-16LE") data.prepend(QByteArray::fromHex("fffe"));
        if (document.encoding == "UTF-16BE") data.prepend(QByteArray::fromHex("feff"));
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        error = file.errorString(); return false;
    }
    written = data;
    return true;
}
