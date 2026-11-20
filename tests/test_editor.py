# ============================================================
# 文件名称：test_editor.py
# 生成日期：2026-10-31
# 修改日期：2026-11-20
# 版本信息：v0.261115
# 作    者：JUN
# 备    注：本程序由AI辅助完成。
#           文件功能：验证 editor 相关功能。
#           文件用途：用于开发验证及功能回归。
#           注意事项：使用隔离测试环境，避免操作正式业务数据。
# ============================================================
import codecs
import os
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from PySide6.QtWidgets import QApplication, QMessageBox
from documents import read_document, write_document
from main import EditorWindow

APP = QApplication.instance() or QApplication([])


class DocumentTests(unittest.TestCase):
    def test_encodings_and_newlines_roundtrip(self):
        with tempfile.TemporaryDirectory() as folder:
            for encoding, newline in [('utf-8', '\n'), ('utf-8-sig', '\r\n'), ('utf-16', '\r\n'), ('gb18030', '\r\n')]:
                path = Path(folder) / '中文文档.md'
                raw = ('# 中文' + newline + '测试').encode(encoding)
                path.write_bytes(raw)
                text, actual, ending = read_document(path)
                self.assertEqual(text, '# 中文\n测试')
                self.assertEqual(actual, encoding)
                self.assertEqual(ending, newline)
                write_document(path, text, actual, ending)
                self.assertEqual(path.read_bytes(), raw)

    def test_failure_preserves_original(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'original.md'
            path.write_text('original', encoding='utf-8')
            with patch('documents.os.replace', side_effect=PermissionError('locked')):
                with self.assertRaises(PermissionError):
                    write_document(path, 'changed')
            self.assertEqual(path.read_text(), 'original')
            self.assertEqual(len(list(Path(folder).iterdir())), 1)

    def test_binary_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'binary'
            path.write_bytes(b'abc\x00def')
            with self.assertRaises(ValueError):
                read_document(path)


class EditorTests(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory()
        self.window = EditorWindow()

    def tearDown(self):
        for index in range(self.window.tabs.count()):
            self.window.tabs.widget(index).editor.document().setModified(False)
        self.window.close()
        self.window.deleteLater()
        APP.processEvents()
        self.folder.cleanup()

    def test_markdown_preview_and_plaintext(self):
        tab = self.window.new_document('# 标题\n\n**加粗**\n\n| A | B |\n|---|---|\n| 1 | 2 |')
        APP.processEvents()
        self.assertIn('<h1', tab.preview.document().toHtml())
        self.assertIn('<table', tab.preview.document().toHtml())
        path = Path(self.folder.name) / 'text.txt'
        path.write_text('# plain', encoding='utf-8')
        other = self.window.open_path(path)
        self.assertNotIn('<h1', other.preview.document().toHtml())
        self.assertIs(self.window.open_path(path), other)
        self.assertEqual(self.window.tabs.count(), 2)

    def test_emoji_shortcodes_heading_and_source_preserved(self):
        source = '## :clapper: 课程内容大纲\n\n😀 :smile: :+1: :unknown_alias_xyz:'
        tab = self.window.new_document(source)
        plain = tab.preview.toPlainText()
        self.assertIn('🎬 课程内容大纲', plain)
        self.assertIn('😀 😄 👍 :unknown_alias_xyz:', plain)
        self.assertIn('<h2', tab.preview.document().toHtml())
        self.assertEqual(tab.editor.toPlainText(), source)
        self.assertFalse(tab.editor.document().isModified())

    def test_emoji_code_urls_and_plaintext_preserved(self):
        source = '`:clapper:`\n\n```text\n:clapper:\n```\n\n    :smile:\n\n[链接 :clapper:](https://example.com/:clapper:)'
        tab = self.window.new_document(source)
        self.assertEqual(tab.preview.toPlainText().count(':clapper:'), 2)
        self.assertIn(':smile:', tab.preview.toPlainText())
        self.assertIn('链接 🎬', tab.preview.toPlainText())
        self.assertIn('https://example.com/:clapper:', tab.preview.document().toHtml())
        path = Path(self.folder.name) / 'emoji.txt'
        path.write_text('## :clapper: 课程内容大纲', encoding='utf-8')
        plain = self.window.open_path(path)
        self.assertEqual(plain.preview.toPlainText(), '## :clapper: 课程内容大纲')

    def test_save_replace_undo_and_external_conflict(self):
        path = Path(self.folder.name) / '中文.md'
        path.write_bytes(codecs.BOM_UTF8 + '你好\r\n你好'.encode())
        tab = self.window.open_path(path)
        self.window.query.setText('你好')
        self.window.replacement.setText('世界你好')
        self.window.replace_all()
        self.assertEqual(tab.editor.toPlainText(), '世界你好\n世界你好')
        tab.editor.undo()
        self.assertEqual(tab.editor.toPlainText(), '你好\n你好')
        tab.editor.appendPlainText('修改')
        self.assertTrue(self.window.save())
        self.assertEqual(path.read_bytes(), codecs.BOM_UTF8 + '你好\r\n你好\r\n修改'.encode())
        path.write_text('external', encoding='utf-8')
        with patch.object(QMessageBox, 'question', return_value=QMessageBox.StandardButton.No):
            self.assertFalse(self.window.save())
        self.assertEqual(path.read_text(), 'external')

    def test_escaped_slides_link_entities_and_code(self):
        source = r'&#x20;[Slides]\(./00Summary/)'
        tab = self.window.new_document(source)
        self.assertEqual(tab.preview.toPlainText().strip(), 'Slides')
        self.assertIn('href="./00Summary/"', tab.preview.document().toHtml())
        self.assertEqual(tab.editor.toPlainText(), source)
        tab.editor.setPlainText('`' + source + '`\n\n```md\n' + source + '\n```')
        tab.render()
        self.assertNotIn('href="./00Summary/"', tab.preview.document().toHtml())
        self.assertEqual(tab.preview.toPlainText().count(source), 2)

    def test_relative_directory_link_opens_readme(self):
        from PySide6.QtCore import QUrl
        root = Path(self.folder.name)
        folder = root / '00Summary'
        folder.mkdir()
        (folder / 'README.md').write_text('# Slides', encoding='utf-8')
        path = root / 'index.md'
        path.write_text(r'&#x20;[Slides]\(./00Summary/)', encoding='utf-8')
        tab = self.window.open_path(path)
        tab.open_link(QUrl('./00Summary/'))
        self.assertEqual(self.window.current().path, (folder / 'README.md').resolve())

    def test_unsaved_close_cancel_and_discard(self):
        tab = self.window.new_document()
        tab.editor.insertPlainText('unsaved')
        with patch.object(QMessageBox, 'question', return_value=QMessageBox.StandardButton.Cancel):
            self.window.close_tab(0)
        self.assertEqual(self.window.tabs.count(), 1)
        self.assertIs(self.window.current(), tab)
        with patch.object(QMessageBox, 'question', return_value=QMessageBox.StandardButton.Discard):
            self.window.close_tab(0)
        self.assertEqual(self.window.tabs.count(), 1)
        self.assertIsNot(self.window.current(), tab)


if __name__ == '__main__':
    unittest.main()
