# ============================================================
# 文件名称：preview.py
# 生成日期：2026-10-31
# 修改日期：2026-11-20
# 版本信息：v0.261115
# 作    者：JUN
# 备    注：本程序由AI辅助完成。
#           文件功能：实现 preview 模块相关功能。
#           文件用途：供工具的 TOOLS/MARKDOWN/preview.py 模块使用。
#           注意事项：运行前检查依赖与配置；涉及数据或服务变更时先备份并验证。
# ============================================================
"""Markdown preview extensions without changing the source document."""
import re

import emoji
from PySide6.QtGui import QColor, QFont, QTextCursor, QTextFormat


SHORTCODE = re.compile(r':[A-Za-z0-9_+\-]+:')
ESCAPED_LINK = re.compile(r'\[([^\[\]\n]+)\]\(((?:\./|\.\./|https?://)[^\s()]+)\)')


def readable_format(block, fmt):
    return (not block.blockFormat().hasProperty(QTextFormat.Property.BlockCodeLanguage)
            and not fmt.fontFixedPitch()
            and fmt.font().styleHint() not in (QFont.StyleHint.Courier, QFont.StyleHint.Monospace))


def restore_escaped_links(document):
    """Compatibility for exported Markdown containing [label]\\(relative/path)."""
    replacements = []
    block = document.begin()
    while block.isValid():
        iterator = block.begin()
        while not iterator.atEnd():
            fragment = iterator.fragment()
            fmt = fragment.charFormat()
            if readable_format(block, fmt) and not fmt.isAnchor():
                text = fragment.text()
                for match in ESCAPED_LINK.finditer(text):
                    start = fragment.position() + len(text[:match.start()].encode('utf-16-le')) // 2
                    length = len(match.group().encode('utf-16-le')) // 2
                    replacements.append((start, length, match.group(1), match.group(2), fmt))
            iterator += 1
        block = block.next()
    for start, length, label, target, fmt in reversed(replacements):
        cursor = QTextCursor(document)
        cursor.setPosition(start)
        cursor.setPosition(start + length, QTextCursor.MoveMode.KeepAnchor)
        fmt.setAnchor(True)
        fmt.setAnchorHref(target)
        fmt.setForeground(QColor('#1769aa'))
        fmt.setFontUnderline(True)
        cursor.insertText(label, fmt)


def render_markdown(document, source):
    document.setMarkdown(source)
    restore_escaped_links(document)
    replacements = []
    block = document.begin()
    while block.isValid():
        # Qt marks fenced/indented code blocks and uses Courier for inline code.
        code_block = block.blockFormat().hasProperty(QTextFormat.Property.BlockCodeLanguage)
        iterator = block.begin()
        while not iterator.atEnd():
            fragment = iterator.fragment()
            fmt = fragment.charFormat()
            if not code_block and not fmt.fontFixedPitch() and fmt.font().styleHint() not in (QFont.StyleHint.Courier, QFont.StyleHint.Monospace):
                text = fragment.text()
                for match in SHORTCODE.finditer(text):
                    result = emoji.emojize(match.group(), language='alias')
                    if result != match.group():
                        # QTextCursor positions count UTF-16 units, unlike Python indices.
                        start = fragment.position() + len(text[:match.start()].encode('utf-16-le')) // 2
                        replacements.append((start, len(match.group()), result, fmt))
            iterator += 1
        block = block.next()
    for start, length, result, fmt in reversed(replacements):
        cursor = QTextCursor(document)
        cursor.setPosition(start)
        cursor.setPosition(start + length, QTextCursor.MoveMode.KeepAnchor)
        fmt.setFontFamilies(['Segoe UI Emoji', 'Segoe UI Symbol'])
        cursor.insertText(result, fmt)
