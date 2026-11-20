# ============================================================
# 文件名称：main.py
# 生成日期：2026-10-31
# 修改日期：2026-11-20
# 版本信息：v0.261115
# 作    者：JUN
# 备    注：本程序由AI辅助完成。
#           文件功能：组织程序入口、服务初始化及请求处理。
#           文件用途：供工具的 TOOLS/MARKDOWN/main.py 模块使用。
#           注意事项：运行前检查依赖与配置；涉及数据或服务变更时先备份并验证。
# ============================================================
"""Windows desktop Markdown and plain-text editor."""
import sys
from pathlib import Path

from PySide6.QtCore import Qt, QTimer, QUrl, QSettings
from PySide6.QtGui import QAction, QFont, QKeySequence, QTextCursor, QTextDocument, QDesktopServices
from PySide6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QPlainTextEdit, QTextBrowser, QSplitter,
    QTabWidget, QVBoxLayout, QHBoxLayout, QLabel, QLineEdit, QPushButton,
    QFileDialog, QMessageBox, QToolBar, QComboBox, QCheckBox,
)

from documents import read_document, write_document
from preview import render_markdown

FILTER = 'Markdown 文件 (*.md *.markdown *.mdown);;文本文件 (*.txt *.log *.ini *.cfg *.csv *.json *.yaml *.yml *.py);;所有文件 (*)'
WELCOME = '''# 欢迎使用 MARKDOWN

在左侧编辑，右侧实时预览。支持打开 Markdown 和文本文件。

## 常用功能

- **Ctrl+O**：打开文件（支持多选）
- **Ctrl+S**：保存
- **Ctrl+Shift+S**：另存为
- **Ctrl+F / Ctrl+H**：查找 / 替换
- **Ctrl+Z / Ctrl+Y**：撤销 / 重做

| 功能 | 支持 |
| --- | --- |
| 标题、列表、表格 | 是 |
| 引用、代码块、本地图片 | 是 |

> 文件使用本地保存，无需启动服务器。

```python
print("Hello, Markdown!")
```
'''


class DocumentTab(QWidget):
    def __init__(self, window, path=None, text='', encoding='utf-8', newline='\n'):
        super().__init__()
        self.window, self.path = window, path
        self.encoding, self.newline = encoding, newline
        self.snapshot = path.read_bytes() if path else None
        self.editor = QPlainTextEdit()
        self.editor.setFont(QFont('Consolas', 12))
        self.editor.setTabStopDistance(self.editor.fontMetrics().horizontalAdvance(' ') * 4)
        self.editor.setPlainText(text)
        self.preview = QTextBrowser()
        self.preview.setOpenLinks(False)
        self.preview.anchorClicked.connect(self.open_link)
        self.preview.setStyleSheet('QTextBrowser {background: #ffffff; padding: 20px; border: 0;}')
        self.preview.document().setDefaultFont(QFont('Microsoft YaHei UI', 11))
        self.splitter = QSplitter()
        self.splitter.addWidget(self.editor)
        self.splitter.addWidget(self.preview)
        self.splitter.setSizes([600, 600])
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.splitter)
        self.timer = QTimer(self)
        self.timer.setSingleShot(True)
        self.timer.setInterval(220)
        self.timer.timeout.connect(self.render)
        self.editor.textChanged.connect(self.changed)
        self.editor.cursorPositionChanged.connect(window.update_status)
        self.editor.document().modificationChanged.connect(lambda _: window.update_titles())
        self.render()

    def changed(self):
        self.timer.start()
        self.window.update_status()

    def render(self):
        position = self.preview.verticalScrollBar().value()
        self.preview.document().setBaseUrl(QUrl.fromLocalFile(str(self.path.parent) + '/') if self.path else QUrl())
        if self.path and self.path.suffix.lower() not in ('.md', '.markdown', '.mdown'):
            self.preview.setPlainText(self.editor.toPlainText())
        else:
            render_markdown(self.preview.document(), self.editor.toPlainText())
        self.preview.verticalScrollBar().setValue(position)

    def open_link(self, url):
        if url.isRelative() and not url.path():
            self.preview.scrollToAnchor(url.fragment())
            return
        if url.isRelative() and self.path:
            url = QUrl.fromLocalFile(str(self.path.parent) + '/').resolved(url)
        if url.scheme() in ('http', 'https', 'mailto'):
            QDesktopServices.openUrl(url)
        elif url.isLocalFile():
            target = Path(url.toLocalFile())
            if target.is_dir():
                for name in ('README.md', 'readme.md', 'index.md', 'README.markdown'):
                    if (target / name).is_file():
                        self.window.open_path(target / name)
                        return
                QDesktopServices.openUrl(url)
                return
            if target.suffix.lower() in ('.md', '.markdown', '.txt', '.mdown'):
                self.window.open_path(target)


class EditorWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('MARKDOWN v0.261115 · 版权归本人所有。')
        self.resize(1280, 820)
        self.settings = QSettings('LocalTools', 'MarkdownEditor')
        self.tabs = QTabWidget()
        self.tabs.setTabsClosable(True)
        self.tabs.setMovable(True)
        self.tabs.tabCloseRequested.connect(self.close_tab)
        self.tabs.currentChanged.connect(self.current_changed)
        self.search = QWidget()
        bar = QHBoxLayout(self.search)
        self.query = QLineEdit()
        self.query.setPlaceholderText('查找文本')
        self.replacement = QLineEdit()
        self.replacement.setPlaceholderText('替换为')
        self.case = QCheckBox('区分大小写')
        for widget in (QLabel('查找'), self.query, self.replacement, self.case):
            bar.addWidget(widget)
        for label, callback in [('上一个', lambda: self.find(True)), ('下一个', self.find), ('替换', self.replace), ('全部替换', self.replace_all), ('关闭', self.search.hide)]:
            button = QPushButton(label)
            button.clicked.connect(lambda checked=False, cb=callback: cb())
            bar.addWidget(button)
        self.query.returnPressed.connect(self.find)
        self.search.hide()
        root = QWidget()
        layout = QVBoxLayout(root)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.addWidget(self.tabs)
        layout.addWidget(self.search)
        self.setCentralWidget(root)
        self.toolbar = QToolBar('常用工具')
        self.toolbar.setMovable(False)
        self.addToolBar(self.toolbar)
        file_menu = self.menuBar().addMenu('文件')
        edit_menu = self.menuBar().addMenu('编辑')
        view_menu = self.menuBar().addMenu('视图')
        for menu, label, shortcut, callback, toolbar in [
            (file_menu, '新建', 'Ctrl+N', self.new_document, True),
            (file_menu, '打开', 'Ctrl+O', self.open_files, True),
            (file_menu, '保存', 'Ctrl+S', self.save, True),
            (file_menu, '另存为', 'Ctrl+Shift+S', lambda: self.save(True), True),
            (file_menu, '导出 HTML', '', self.export_html, False),
            (file_menu, '关闭文档', 'Ctrl+W', lambda: self.close_tab(self.tabs.currentIndex()), False),
            (file_menu, '退出', 'Alt+F4', self.close, False),
            (edit_menu, '撤销', 'Ctrl+Z', lambda: self.current().editor.undo(), False),
            (edit_menu, '重做', 'Ctrl+Y', lambda: self.current().editor.redo(), False),
            (edit_menu, '查找', 'Ctrl+F', self.show_search, True),
            (edit_menu, '替换', 'Ctrl+H', lambda: self.show_search(True), False),
            (view_menu, '放大', 'Ctrl++', lambda: self.zoom(1), False),
            (view_menu, '缩小', 'Ctrl+-', lambda: self.zoom(-1), False),
        ]:
            action = QAction(label, self)
            if shortcut:
                action.setShortcut(QKeySequence(shortcut))
            action.triggered.connect(lambda checked=False, cb=callback: cb())
            menu.addAction(action)
            if toolbar:
                self.toolbar.addAction(action)
        self.toolbar.addSeparator()
        self.mode = QComboBox()
        self.mode.addItems(['编辑 + 预览', '仅编辑', '仅预览'])
        self.mode.currentIndexChanged.connect(self.apply_mode)
        self.toolbar.addWidget(self.mode)
        self.setAcceptDrops(True)
        self.setStyleSheet('QMainWindow {background:#f3f6fa;} QToolBar {spacing:8px; padding:8px;} QTabWidget::pane {border:1px solid #dce3ed;} QPushButton {padding:5px 10px;} QLineEdit {padding:5px;}')
        geometry = self.settings.value('geometry')
        if geometry:
            self.restoreGeometry(geometry)

    def current(self):
        return self.tabs.currentWidget()

    def new_document(self, text=''):
        tab = DocumentTab(self, text=text)
        self.tabs.addTab(tab, '未命名.md')
        self.tabs.setCurrentWidget(tab)
        tab.editor.setFocus()
        return tab

    def open_files(self):
        paths, _ = QFileDialog.getOpenFileNames(self, '打开 Markdown 或文本文件', self.settings.value('directory', ''), FILTER)
        for path in paths:
            self.open_path(Path(path))

    def open_path(self, path):
        path = Path(path).resolve()
        for index in range(self.tabs.count()):
            if self.tabs.widget(index).path == path:
                self.tabs.setCurrentIndex(index)
                return self.tabs.widget(index)
        try:
            text, encoding, newline = read_document(path)
            tab = DocumentTab(self, path, text, encoding, newline)
            self.tabs.addTab(tab, path.name)
            self.tabs.setCurrentWidget(tab)
            self.settings.setValue('directory', str(path.parent))
            return tab
        except (OSError, UnicodeError, ValueError) as exc:
            QMessageBox.warning(self, '打开失败', str(exc))

    def save(self, save_as=False, tab=None):
        tab = tab or self.current()
        if not tab:
            return False
        path = tab.path
        if save_as or path is None:
            name, _ = QFileDialog.getSaveFileName(self, '保存文件', str(path) if path else str(Path(self.settings.value('directory', str(Path.home()))) / '未命名.md'), FILTER)
            if not name:
                return False
            path = Path(name).resolve()
            for index in range(self.tabs.count()):
                other = self.tabs.widget(index)
                if other is not tab and other.path == path:
                    QMessageBox.warning(self, '无法保存', '此文件已在另一个页签打开，请切换到该页签编辑。')
                    return False
        try:
            if path == tab.path and (not path.exists() or path.read_bytes() != tab.snapshot):
                answer = QMessageBox.question(self, '文件已在外部更改', '磁盘文件已更改或删除，是否用当前编辑内容覆盖？', QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No)
                if answer != QMessageBox.StandardButton.Yes:
                    return False
            write_document(path, tab.editor.toPlainText(), tab.encoding, tab.newline)
            tab.path, tab.snapshot = path, path.read_bytes()
            tab.editor.document().setModified(False)
            self.settings.setValue('directory', str(path.parent))
            tab.render()
            self.update_titles()
            self.update_status()
            return True
        except (OSError, UnicodeError, ValueError) as exc:
            QMessageBox.warning(self, '保存失败', str(exc))
            return False

    def confirm_close(self, tab):
        if not tab.editor.document().isModified():
            return True
        answer = QMessageBox.question(self, '保存更改', f'是否保存 {tab.path.name if tab.path else "未命名文档"} 的修改？', QMessageBox.StandardButton.Save | QMessageBox.StandardButton.Discard | QMessageBox.StandardButton.Cancel)
        return answer == QMessageBox.StandardButton.Discard or (answer == QMessageBox.StandardButton.Save and self.save(tab=tab))

    def close_tab(self, index):
        if index < 0:
            return
        tab = self.tabs.widget(index)
        if self.confirm_close(tab):
            self.tabs.removeTab(index)
            tab.deleteLater()
            if not self.tabs.count():
                self.new_document()

    def closeEvent(self, event):
        for index in range(self.tabs.count()):
            tab = self.tabs.widget(index)
            if not self.confirm_close(tab):
                event.ignore()
                return
        self.settings.setValue('geometry', self.saveGeometry())
        event.accept()

    def update_titles(self):
        for index in range(self.tabs.count()):
            tab = self.tabs.widget(index)
            self.tabs.setTabText(index, ('● ' if tab.editor.document().isModified() else '') + (tab.path.name if tab.path else '未命名.md'))
            self.tabs.setTabToolTip(index, str(tab.path) if tab.path else '尚未保存')

    def current_changed(self, *_):
        self.apply_mode()
        self.update_status()

    def apply_mode(self, *_):
        tab = self.current()
        if tab:
            tab.editor.setVisible(self.mode.currentIndex() != 2)
            tab.preview.setVisible(self.mode.currentIndex() != 1)

    def update_status(self):
        tab = self.current()
        if tab:
            cursor = tab.editor.textCursor()
            self.statusBar().showMessage(f'{tab.path or "未命名文档"}  |  {tab.encoding.upper()}  |  {"CRLF" if tab.newline == chr(13)+chr(10) else "LF"}  |  行 {cursor.blockNumber()+1}，列 {cursor.positionInBlock()+1}  |  {len(tab.editor.toPlainText())} 字符')

    def show_search(self, replace=False):
        self.search.show()
        self.replacement.setVisible(replace)
        self.query.setFocus()
        self.query.selectAll()

    def find(self, backwards=False):
        tab = self.current()
        if not tab or not self.query.text():
            return False
        flags = QTextDocument.FindFlag(0)
        if self.case.isChecked():
            flags |= QTextDocument.FindFlag.FindCaseSensitively
        if backwards:
            flags |= QTextDocument.FindFlag.FindBackward
        if tab.editor.find(self.query.text(), flags):
            return True
        original = tab.editor.textCursor()
        cursor = tab.editor.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.End if backwards else QTextCursor.MoveOperation.Start)
        tab.editor.setTextCursor(cursor)
        found = tab.editor.find(self.query.text(), flags)
        if not found:
            tab.editor.setTextCursor(original)
            self.statusBar().showMessage('未找到匹配文本', 3000)
        return found

    def replace(self):
        tab = self.current()
        if not tab or not self.query.text():
            return
        selected = tab.editor.textCursor().selectedText().replace('\u2029', '\n')
        matches = selected == self.query.text() if self.case.isChecked() else selected.casefold() == self.query.text().casefold()
        if matches:
            tab.editor.textCursor().insertText(self.replacement.text())
        self.find()

    def replace_all(self):
        tab = self.current()
        if not tab or not self.query.text():
            return
        document = tab.editor.document()
        flags = QTextDocument.FindFlag.FindCaseSensitively if self.case.isChecked() else QTextDocument.FindFlag(0)
        transaction = QTextCursor(document)
        transaction.beginEditBlock()
        cursor = QTextCursor(document)
        count = 0
        while True:
            cursor = document.find(self.query.text(), cursor, flags)
            if cursor.isNull():
                break
            cursor.insertText(self.replacement.text())
            count += 1
        transaction.endEditBlock()
        self.statusBar().showMessage(f'已替换 {count} 处', 4000)

    def zoom(self, delta):
        tab = self.current()
        if tab:
            tab.editor.zoomIn(delta)
            tab.preview.zoomIn(delta)

    def export_html(self):
        tab = self.current()
        if not tab:
            return
        name, _ = QFileDialog.getSaveFileName(self, '导出 HTML', str(tab.path.with_suffix('.html')) if tab.path else '文档.html', 'HTML 文件 (*.html)')
        if name:
            try:
                tab.render()
                write_document(name, tab.preview.document().toHtml())
            except (OSError, UnicodeError) as exc:
                QMessageBox.warning(self, '导出失败', str(exc))

    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()

    def dropEvent(self, event):
        for url in event.mimeData().urls():
            if url.isLocalFile():
                self.open_path(Path(url.toLocalFile()))


def main():
    app = QApplication(sys.argv)
    app.setApplicationName('MARKDOWN')
    app.setApplicationVersion('v0.261115')
    app.setStyle('Fusion')
    window = EditorWindow()
    for argument in sys.argv[1:]:
        window.open_path(Path(argument))
    if not window.tabs.count():
        window.new_document(WELCOME)
    window.show()
    sys.exit(app.exec())


if __name__ == '__main__':
    main()
