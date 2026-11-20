// ============================================================
// 文件名称：rendering.cpp
// 生成日期：2026-10-31
// 修改日期：2026-11-20
// 版本信息：v0.261115
// 作    者：JUN
// 备    注：本程序由AI辅助完成。
//           文件功能：实现 rendering 模块相关功能。
//           文件用途：供工具的 TOOLS/MARKDOWN/cpp/rendering.cpp 模块使用。
//           注意事项：运行前检查依赖与配置；涉及数据或服务变更时先备份并验证。
// ============================================================
#include "rendering.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>
#include <QColor>
#include <QTextTable>

static bool prose(const QTextBlock &block, const QTextCharFormat &format) {
    return !block.blockFormat().hasProperty(QTextFormat::BlockCodeLanguage)
        && !format.fontFixedPitch() && format.font().styleHint() != QFont::Courier
        && format.font().styleHint() != QFont::Monospace;
}

struct Replacement { int start; int length; QString text; QTextCharFormat format; };

static void apply(QTextDocument *document, const QList<Replacement> &items) {
    for (auto it = items.crbegin(); it != items.crend(); ++it) {
        QTextCursor cursor(document);
        cursor.setPosition(it->start);
        cursor.setPosition(it->start + it->length, QTextCursor::KeepAnchor);
        cursor.insertText(it->text, it->format);
    }
}

void renderMarkdown(QTextDocument *document, const QString &source) {
    Q_INIT_RESOURCE(resources);
    document->setMarkdown(source);
    static const auto emojis = [] {
        QFile file(":/data/emoji.json"); file.open(QIODevice::ReadOnly);
        return QJsonDocument::fromJson(file.readAll()).object();
    }();
    static const QRegularExpression link(R"(\[([^\[\]\n]+)\]\(((?:\./|\.\./|https?://)[^\s()]+)\))");
    static const QRegularExpression shortcode(R"(:[A-Za-z0-9_+\-]+:)");
    QList<Replacement> items;
    for (auto block = document->begin(); block.isValid(); block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            auto fragment = it.fragment(); auto format = fragment.charFormat();
            if (!prose(block, format) || format.isAnchor()) continue;
            auto matches = link.globalMatch(fragment.text());
            while (matches.hasNext()) {
                auto match = matches.next();
                auto fmt = format;
                fmt.setAnchor(true); fmt.setAnchorHref(match.captured(2));
                fmt.setForeground(QColor("#467da4")); fmt.setFontUnderline(true);
                items.append({fragment.position() + int(match.capturedStart()), int(match.capturedLength()), match.captured(1), fmt});
            }
        }
    }
    apply(document, items); items.clear();
    for (auto block = document->begin(); block.isValid(); block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            auto fragment = it.fragment(); auto format = fragment.charFormat();
            if (!prose(block, format)) continue;
            auto matches = shortcode.globalMatch(fragment.text());
            while (matches.hasNext()) {
                auto match = matches.next();
                auto replacement = emojis.value(match.captured()).toString();
                if (replacement.isEmpty()) continue;
                auto fmt = format; fmt.setFontFamilies({"Segoe UI Emoji", "Segoe UI Symbol"});
                items.append({fragment.position() + int(match.capturedStart()), int(match.capturedLength()), replacement, fmt});
            }
        }
    }
    apply(document, items);
    for (auto block = document->begin(); block.isValid(); block = block.next()) {
        QTextCursor cursor(block);
        auto format = block.blockFormat();
        format.setLineHeight(150, QTextBlockFormat::ProportionalHeight);
        format.setTopMargin(format.headingLevel() ? 22 : 3);
        format.setBottomMargin(format.headingLevel() ? 12 : 8);
        cursor.setBlockFormat(format);
    }
    const auto styleTables = [](QTextFrame *frame, const auto &recurse) -> void {
        for (auto child : frame->childFrames()) {
            if (auto table = qobject_cast<QTextTable *>(child)) {
                auto format = table->format();
                format.setBorder(0.6); format.setBorderBrush(QColor("#b7bdc4"));
                format.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
                format.setBorderCollapse(true); format.setCellPadding(9); format.setCellSpacing(0);
                format.setWidth(QTextLength(QTextLength::PercentageLength,100));
                table->setFormat(format);
            }
            recurse(child,recurse);
        }
    };
    styleTables(document->rootFrame(),styleTables);
}
