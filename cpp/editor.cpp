#include "editor.h"
#include "rendering.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QScrollBar>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QTextBlock>
#include <QTextList>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QPushButton>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QResizeEvent>

class WritingEdit : public QTextEdit {
protected:
    void resizeEvent(QResizeEvent *event) override {
        const int margin=qMax(0,(width()-940)/2);
        setViewportMargins(margin,0,margin,0);
        QTextEdit::resizeEvent(event);
    }
};

static const QString filters = "Markdown 文件 (*.md *.markdown *.mdown);;文本文件 (*.txt *.log *.ini *.cfg *.csv *.json *.yaml *.yml *.cpp *.h *.py);;所有文件 (*)";

Page::Page(const FileDocument &value, QWidget *parent) : QStackedWidget(parent), file(value) {
    visual = new WritingEdit;
    visual->setObjectName("paper");
    visual->setAcceptRichText(false);
    visual->setFont(QFont("Microsoft YaHei UI", 12));
    visual->document()->setDocumentMargin(38);
    visual->document()->setDefaultStyleSheet("h1 {font-size:30pt; margin-top:26px; margin-bottom:18px;} h2 {font-size:23pt; margin-top:26px; margin-bottom:14px;} h3 {font-size:17pt;} p {line-height:160%; margin-bottom:14px;} blockquote {color:#75808a;} a {color:#467da4;} pre {background:#f4f5f7;} th,td {padding:8px;}");
    visual->document()->setBaseUrl(QUrl::fromLocalFile(QFileInfo(file.path).absolutePath() + '/'));
    source = new QPlainTextEdit;
    source->setObjectName("source");
    source->setFont(QFont("Consolas", 12));
    source->setTabStopDistance(source->fontMetrics().horizontalAdvance(' ') * 4);
    source->setPlainText(file.text);
    if (isMarkdown(file.path)) renderMarkdown(visual->document(), file.text);
    else visual->setPlainText(file.text);
    addWidget(visual); addWidget(source);
    source->document()->setModified(false); visual->document()->setModified(false);
    connect(source, &QPlainTextEdit::textChanged, this, [this] {
        if (updating) return;
        modified = true; sourceDirty = true; visualDirty = false;
    });
    connect(visual, &QTextEdit::textChanged, this, [this] {
        if (updating) return;
        modified = true; visualDirty = true; sourceDirty = false;
    });
}

QString Page::markdown() {
    if (visualDirty) {
        file.text = isMarkdown(file.path) ? visual->document()->toMarkdown() : visual->toPlainText();
    } else if (sourceDirty) file.text = source->toPlainText();
    return file.text;
}

void Page::syncVisual() {
    if (!sourceDirty) return;
    markdown(); updating = true;
    if (isMarkdown(file.path)) renderMarkdown(visual->document(), file.text);
    else visual->setPlainText(file.text);
    sourceDirty = false; visualDirty = false; updating = false;
}

void Page::setSourceMode(bool enabled) {
    if (enabled && currentWidget() != source && visualDirty) {
        markdown(); updating = true;
        source->setPlainText(file.text);
        visualDirty = false; sourceDirty = false; updating = false;
    }
    if (!enabled) syncVisual();
    setCurrentWidget(enabled ? static_cast<QWidget *>(source) : static_cast<QWidget *>(visual));
}

QTextEdit *Page::activeRich() const { return currentWidget() == visual ? visual : nullptr; }

EditorWindow::EditorWindow() : settings("LocalTools", "MarkdownNative") {
    setWindowTitle("MARKDOWN v0.261115 · C++ 编辑器"); resize(1320, 880); setMinimumSize(720, 480);
    statusBar()->addPermanentWidget(new QLabel("v0.261115 · 版权归本人所有。"));
    setAcceptDrops(true);
    auto root = new QWidget; auto layout = new QVBoxLayout(root); layout->setContentsMargins(0,0,0,0); layout->setSpacing(0);
    auto splitter = new QSplitter;
    auto sidebar = new QWidget; sidebar->setObjectName("sidebar");
    auto sidebarLayout = new QVBoxLayout(sidebar); sidebarLayout->setContentsMargins(18,24,10,12);
    auto brand = new QLabel("MARKDOWN"); brand->setObjectName("brand"); sidebarLayout->addWidget(brand);
    auto subtitle = new QLabel("文档大纲"); subtitle->setObjectName("subtitle"); sidebarLayout->addWidget(subtitle);
    outline = new QTreeWidget; outline->setHeaderHidden(true); outline->setIndentation(14); outline->setFrameShape(QFrame::NoFrame);
    sidebarLayout->addWidget(outline);
    auto note = new QLabel("C++ / Qt  ·  本地文档"); note->setObjectName("subtitle"); sidebarLayout->addWidget(note);
    sidebar->setMinimumWidth(175);
    tabs = new QTabWidget; tabs->setObjectName("documents"); tabs->setTabsClosable(true); tabs->setMovable(true); tabs->setDocumentMode(true);
    splitter->addWidget(sidebar); splitter->addWidget(tabs); splitter->setSizes({235,1085}); splitter->setStretchFactor(1,1);
    layout->addWidget(splitter);
    findBar = new QWidget; findBar->setObjectName("findbar"); auto findLayout = new QHBoxLayout(findBar);
    query = new QLineEdit; query->setPlaceholderText("查找文本"); replacement = new QLineEdit; replacement->setPlaceholderText("替换为");
    matchCase = new QCheckBox("区分大小写");
    findLayout->addWidget(query); findLayout->addWidget(replacement); findLayout->addWidget(matchCase);
    auto button = [&](const QString &label, auto callback) { auto b = new QPushButton(label); findLayout->addWidget(b); connect(b,&QPushButton::clicked,this,callback); };
    button("上一个",[this]{findNext(true);}); button("下一个",[this]{findNext();}); button("替换",[this]{replaceOne();}); button("全部替换",[this]{replaceAll();}); button("×",[this]{findBar->hide();});
    connect(query,&QLineEdit::returnPressed,this,[this]{findNext();});
    layout->addWidget(findBar); findBar->hide(); setCentralWidget(root);
    auto toolbar = addToolBar("文档工具"); toolbar->setMovable(false); toolbar->setObjectName("toolbar");
    auto fileMenu = menuBar()->addMenu("文件"); auto editMenu = menuBar()->addMenu("编辑"); auto formatMenu = menuBar()->addMenu("段落"); auto viewMenu = menuBar()->addMenu("视图");
    auto action = [&](QMenu *menu,const QString &label,const QKeySequence &key,auto callback,bool tool=false) {
        auto item = new QAction(label,this); if(!key.isEmpty()) item->setShortcut(key);
        connect(item,&QAction::triggered,this,callback); menu->addAction(item); if(tool)toolbar->addAction(item); return item;
    };
    action(fileMenu,"新建",QKeySequence::New,[this]{newPage();},true);
    action(fileMenu,"打开",QKeySequence::Open,[this]{openFiles();},true);
    action(fileMenu,"保存",QKeySequence::Save,[this]{save();},true);
    action(fileMenu,"另存为",QKeySequence::SaveAs,[this]{save(nullptr,true);});
    action(fileMenu,"导出 HTML",{},[this]{exportHtml();});
    action(fileMenu,"关闭文档",QKeySequence::Close,[this]{closeTab(tabs->currentIndex());});
    action(fileMenu,"退出",QKeySequence("Alt+F4"),[this]{close();});
    auto active = [this](auto rich,auto plain) { if(auto p=current()){if(p->activeRich())rich(p->visual);else plain(p->source);} };
    action(editMenu,"撤销",QKeySequence::Undo,[active]{active([](auto e){e->undo();},[](auto e){e->undo();});});
    action(editMenu,"重做",QKeySequence("Ctrl+Y"),[active]{active([](auto e){e->redo();},[](auto e){e->redo();});});
    action(editMenu,"剪切",QKeySequence::Cut,[active]{active([](auto e){e->cut();},[](auto e){e->cut();});});
    action(editMenu,"复制",QKeySequence::Copy,[active]{active([](auto e){e->copy();},[](auto e){e->copy();});});
    action(editMenu,"粘贴",QKeySequence::Paste,[active]{active([](auto e){e->paste();},[](auto e){e->paste();});});
    action(editMenu,"全选",QKeySequence::SelectAll,[active]{active([](auto e){e->selectAll();},[](auto e){e->selectAll();});});
    action(editMenu,"查找",QKeySequence::Find,[this]{showFind(false);},true);
    action(editMenu,"替换",QKeySequence("Ctrl+H"),[this]{showFind(true);});
    toolbar->addSeparator();
    action(formatMenu,"加粗",QKeySequence("Ctrl+B"),[this]{format("bold");},true);
    action(formatMenu,"斜体",QKeySequence("Ctrl+I"),[this]{format("italic");});
    action(formatMenu,"一级标题",QKeySequence("Ctrl+1"),[this]{format("h1");});
    action(formatMenu,"二级标题",QKeySequence("Ctrl+2"),[this]{format("h2");});
    action(formatMenu,"三级标题",QKeySequence("Ctrl+3"),[this]{format("h3");});
    action(formatMenu,"正文",QKeySequence("Ctrl+0"),[this]{format("body");});
    action(formatMenu,"无序列表",{},[this]{format("list");});
    sourceAction=action(viewMenu,"源码模式",QKeySequence("Ctrl+/"),[this](bool checked){toggleSource(checked);},true); sourceAction->setCheckable(true);
    sidebarAction=action(viewMenu,"文档大纲",QKeySequence("Ctrl+Shift+L"),[sidebar](bool checked){sidebar->setVisible(checked);});sidebarAction->setCheckable(true);sidebarAction->setChecked(true);
    focusAction=action(viewMenu,"专注模式",QKeySequence("F8"),[this,toolbar,sidebar](bool checked){sidebar->setVisible(!checked && sidebarAction->isChecked());toolbar->setVisible(!checked);menuBar()->setVisible(!checked);statusBar()->setVisible(!checked);});focusAction->setCheckable(true);
    action(viewMenu,"切换深色 / 浅色",QKeySequence("Ctrl+Shift+D"),[this]{dark=!dark;style();settings.setValue("dark",dark);});
    action(viewMenu,"放大",QKeySequence("Ctrl++"),[active]{active([](auto e){e->zoomIn();},[](auto e){e->zoomIn();});});
    action(viewMenu,"缩小",QKeySequence("Ctrl+-"),[active]{active([](auto e){e->zoomOut();},[](auto e){e->zoomOut();});});
    auto encodingMenu = fileMenu->addMenu("保存编码");
    for(const auto &encoding : {QByteArray("UTF-8"),QByteArray("UTF-16LE"),QByteArray("GB18030")})
        action(encodingMenu,QString::fromLatin1(encoding),{},[this,encoding]{if(auto p=current()){p->file.encoding=encoding;p->file.bom=encoding=="UTF-16LE";p->modified=true;refresh();}});
    info = new QLabel; statusBar()->addPermanentWidget(info,1);
    connect(tabs,&QTabWidget::tabCloseRequested,this,&EditorWindow::closeTab);
    connect(tabs,&QTabWidget::currentChanged,this,[this]{if(auto p=current()){QSignalBlocker b(sourceAction);sourceAction->setChecked(p->currentWidget()==p->source);}refresh();refreshOutline();});
    outlineTimer.setSingleShot(true); outlineTimer.setInterval(350);connect(&outlineTimer,&QTimer::timeout,this,&EditorWindow::refreshOutline);
    connect(outline,&QTreeWidget::itemClicked,this,[this](QTreeWidgetItem *item,int){
        if(auto p=current()) {
            const int pos=item->data(0,Qt::UserRole).toInt();
            if(p->activeRich()){auto c=p->visual->textCursor();c.setPosition(pos);p->visual->setTextCursor(c);p->visual->ensureCursorVisible();p->visual->setFocus();}
            else {auto c=p->source->textCursor();c.setPosition(pos);p->source->setTextCursor(c);p->source->ensureCursorVisible();p->source->setFocus();}
        }
    });
    dark=settings.value("dark",false).toBool();style();
    if(settings.contains("geometry"))restoreGeometry(settings.value("geometry").toByteArray());
}

Page *EditorWindow::current() const { return static_cast<Page *>(tabs->currentWidget()); }

void EditorWindow::addPage(Page *p) {
    tabs->addTab(p,p->file.path.isEmpty()?"未命名.md":QFileInfo(p->file.path).fileName());tabs->setCurrentWidget(p);
    auto changed=[this]{refresh();outlineTimer.start();};
    connect(p->visual,&QTextEdit::textChanged,this,changed);connect(p->source,&QPlainTextEdit::textChanged,this,changed);
    connect(p->visual,&QTextEdit::cursorPositionChanged,this,&EditorWindow::refresh);connect(p->source,&QPlainTextEdit::cursorPositionChanged,this,&EditorWindow::refresh);
    // A click follows links; dragging still selects text for editing.
    p->visual->viewport()->installEventFilter(this);
    p->visual->installEventFilter(this);
    p->visual->setToolTip("单击链接打开目标，拖动选择文字；Ctrl+/ 切换 Markdown 源码");
    p->visual->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(p->visual,&QWidget::customContextMenuRequested,this,[this,p](QPoint point){
        auto menu=p->visual->createStandardContextMenu();auto target=p->visual->anchorAt(point);
        if(!target.isEmpty()){menu->addSeparator();auto item=menu->addAction("打开链接");connect(item,&QAction::triggered,this,[this,target]{navigate(QUrl(target));});}
        menu->exec(p->visual->viewport()->mapToGlobal(point));delete menu;
    });
    refresh();refreshOutline();
}

Page *EditorWindow::newPage(const QString &text) { FileDocument f;f.text=text;auto p=new Page(f);addPage(p);return p; }

Page *EditorWindow::openPath(const QString &path) {
    const auto absolute=QFileInfo(path).absoluteFilePath();
    for(int i=0;i<tabs->count();++i){auto p=static_cast<Page *>(tabs->widget(i));if(p->file.path.compare(absolute,Qt::CaseInsensitive)==0){tabs->setCurrentIndex(i);return p;}}
    FileDocument file;QString error;if(!readDocument(absolute,file,error)){QMessageBox::warning(this,"打开失败",error);return nullptr;}
    auto p=new Page(file);addPage(p);settings.setValue("directory",QFileInfo(absolute).absolutePath());return p;
}

void EditorWindow::openFiles() { for(const auto &path:QFileDialog::getOpenFileNames(this,"打开文档",settings.value("directory").toString(),filters))openPath(path); }

bool EditorWindow::save(Page *p,bool saveAs) {
    if(!p)p=current();if(!p)return false;
    auto path=p->file.path;
    if(saveAs||path.isEmpty()){
        path=QFileDialog::getSaveFileName(this,"保存文档",path.isEmpty()?QDir(settings.value("directory",QDir::homePath()).toString()).filePath("未命名.md"):path,filters);
        if(path.isEmpty())return false;
        path=QFileInfo(path).absoluteFilePath();
        for(int i=0;i<tabs->count();++i){auto other=static_cast<Page *>(tabs->widget(i));if(other!=p&&other->file.path.compare(path,Qt::CaseInsensitive)==0){QMessageBox::warning(this,"无法保存","此文件已在其他页签打开。");return false;}}
    }
    if(path==p->file.path){QFile disk(path);bool exists=disk.open(QIODevice::ReadOnly);if(!exists||disk.readAll()!=p->file.snapshot){if(QMessageBox::question(this,"文件已被外部修改","文件已更改或删除，是否覆盖磁盘文件？",QMessageBox::Yes|QMessageBox::No)!=QMessageBox::Yes)return false;}}
    p->markdown();QByteArray written;QString error;
    if(!writeDocument(p->file,path,written,error)){QMessageBox::warning(this,"保存失败",error);return false;}
    p->file.path=path;p->file.snapshot=written;p->modified=false;
    p->visual->document()->setBaseUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()+'/'));
    settings.setValue("directory",QFileInfo(path).absolutePath());refresh();return true;
}

bool EditorWindow::confirmClose(Page *p) {
    if(!p->modified)return true;
    auto answer=QMessageBox::question(this,"保存修改",QString("是否保存 %1 的修改？").arg(p->file.path.isEmpty()?"未命名文档":QFileInfo(p->file.path).fileName()),QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);
    return answer==QMessageBox::Discard||(answer==QMessageBox::Save&&save(p));
}

void EditorWindow::closeTab(int index) { if(index<0)return;auto p=static_cast<Page *>(tabs->widget(index));if(confirmClose(p)){tabs->removeTab(index);p->deleteLater();if(!tabs->count())newPage();} }
void EditorWindow::closeEvent(QCloseEvent *event) {for(int i=0;i<tabs->count();++i)if(!confirmClose(static_cast<Page *>(tabs->widget(i)))){event->ignore();return;}settings.setValue("geometry",saveGeometry());event->accept();}
void EditorWindow::toggleSource(bool enabled) {if(auto p=current()){p->setSourceMode(enabled);refresh();refreshOutline();}}

void EditorWindow::refresh() {
    for(int i=0;i<tabs->count();++i){auto p=static_cast<Page *>(tabs->widget(i));tabs->setTabText(i,(p->modified?"● ":"")+(p->file.path.isEmpty()?"未命名.md":QFileInfo(p->file.path).fileName()));tabs->setTabToolTip(i,p->file.path);}
    if(auto p=current()){
        auto cursor=p->activeRich()?p->visual->textCursor():p->source->textCursor();
        auto text=p->activeRich()?p->visual->toPlainText():p->source->toPlainText();
        info->setText(QString("  %1    ·    %2    ·    %3 字符    ·    行 %4，列 %5    ·    %6").arg(p->activeRich()?"可视化编辑":"Markdown 源码",QString::fromLatin1(p->file.encoding)).arg(text.size()).arg(cursor.blockNumber()+1).arg(cursor.positionInBlock()+1).arg(p->file.newline=="\r\n"?"CRLF":"LF"));
        setWindowTitle((p->modified?"● ":"")+(p->file.path.isEmpty()?"未命名.md":QFileInfo(p->file.path).fileName())+" — MARKDOWN C++");
    }
}

void EditorWindow::refreshOutline() {
    outline->clear();auto p=current();if(!p)return;
    QList<QTreeWidgetItem *> parents; QList<int> levels;
    auto add=[&](const QString &text,int level,int position){auto item=new QTreeWidgetItem(QStringList(text));item->setData(0,Qt::UserRole,position);while(!levels.isEmpty()&&levels.last()>=level){levels.removeLast();parents.removeLast();}if(parents.isEmpty())outline->addTopLevelItem(item);else parents.last()->addChild(item);parents.append(item);levels.append(level);};
    if(p->activeRich()){
        for(auto block=p->visual->document()->begin();block.isValid();block=block.next())if(int level=block.blockFormat().headingLevel())add(block.text(),level,block.position());
    }else{
        static QRegularExpression heading("^(#{1,6})\\s+(.+)$");
        bool fenced=false;QString marker;
        for(auto block=p->source->document()->begin();block.isValid();block=block.next()){
            auto text=block.text();auto trimmed=text.trimmed();
            if(trimmed.startsWith("```")||trimmed.startsWith("~~~")){if(!fenced){fenced=true;marker=trimmed.left(3);}else if(trimmed.startsWith(marker))fenced=false;continue;}
            if(fenced)continue;auto match=heading.match(text);if(match.hasMatch())add(match.captured(2),match.captured(1).size(),block.position());
        }
    }
    outline->expandAll();
}

void EditorWindow::showFind(bool replace) {findBar->show();replacement->setVisible(replace);query->setFocus();query->selectAll();}

bool EditorWindow::findNext(bool backwards) {
    auto p=current();if(!p||query->text().isEmpty())return false;
    QTextDocument::FindFlags flags;if(backwards)flags|=QTextDocument::FindBackward;if(matchCase->isChecked())flags|=QTextDocument::FindCaseSensitively;
    auto document=p->activeRich()?p->visual->document():p->source->document();
    auto original=p->activeRich()?p->visual->textCursor():p->source->textCursor();
    auto found=document->find(query->text(),original,flags);
    if(found.isNull())found=document->find(query->text(),backwards?document->characterCount()-1:0,flags);
    if(found.isNull()){statusBar()->showMessage("未找到匹配文本",3000);return false;}
    if(p->activeRich())p->visual->setTextCursor(found);else p->source->setTextCursor(found);return true;
}

void EditorWindow::replaceOne() {auto p=current();if(!p||query->text().isEmpty())return;auto c=p->activeRich()?p->visual->textCursor():p->source->textCursor();if(c.selectedText().compare(query->text(),matchCase->isChecked()?Qt::CaseSensitive:Qt::CaseInsensitive)==0)c.insertText(replacement->text());findNext();}

int EditorWindow::replaceAll() {
    auto p=current();if(!p||query->text().isEmpty())return 0;auto document=p->activeRich()?p->visual->document():p->source->document();
    QTextDocument::FindFlags flags;if(matchCase->isChecked())flags|=QTextDocument::FindCaseSensitively;
    QTextCursor transaction(document);transaction.beginEditBlock();QTextCursor cursor(document);int count=0;
    while(true){cursor=document->find(query->text(),cursor,flags);if(cursor.isNull())break;cursor.insertText(replacement->text());++count;}
    transaction.endEditBlock();statusBar()->showMessage(QString("已替换 %1 处").arg(count),4000);return count;
}

void EditorWindow::exportHtml() {
    auto p=current();if(!p)return;auto path=QFileDialog::getSaveFileName(this,"导出 HTML","文档.html","HTML (*.html)");if(path.isEmpty())return;
    QTextDocument document;document.setBaseUrl(p->visual->document()->baseUrl());
    if(isMarkdown(p->file.path))renderMarkdown(&document,p->markdown());else document.setPlainText(p->markdown());
    FileDocument output;output.text=document.toHtml();QString error;QByteArray written;
    if(!writeDocument(output,path,written,error))QMessageBox::warning(this,"导出失败",error);
}

void EditorWindow::navigate(const QUrl &input) {
    auto p=current();if(!p)return;auto url=input;
    if(url.isRelative()&&url.path().isEmpty()){
        for(auto block=p->visual->document()->begin();block.isValid();block=block.next())for(auto it=block.begin();!it.atEnd();++it)if(it.fragment().charFormat().anchorNames().contains(url.fragment())){auto c=p->visual->textCursor();c.setPosition(block.position());p->visual->setTextCursor(c);p->visual->ensureCursorVisible();return;}
        return;
    }
    // Windows drive paths are otherwise parsed as a URL scheme ("c:").
    const auto raw=url.scheme().size()==1 ? url.scheme()+':'+url.path(QUrl::FullyDecoded) : url.toString();
    static const QRegularExpression drivePath("^[A-Za-z]:[/\\\\]");
    if(drivePath.match(raw).hasMatch()||raw.startsWith("\\\\"))url=QUrl::fromLocalFile(QDir::fromNativeSeparators(raw));
    if(url.isRelative()){
        if(p->file.path.isEmpty()){
            QMessageBox::information(this,"无法定位相对链接","请先保存当前文档，再打开相对路径链接。");return;
        }
        // Markdown generated on Windows can contain backslashes in local paths.
        auto path=url.path();path.replace('\\','/');url.setPath(path);
        url=QUrl::fromLocalFile(QFileInfo(p->file.path).absolutePath()+'/').resolved(url);
    }
    auto openExternal=[this](const QUrl &target){if(!QDesktopServices::openUrl(target))QMessageBox::warning(this,"无法打开链接","系统无法打开：\n"+target.toDisplayString());};
    if(url.scheme()=="https"||url.scheme()=="http"||url.scheme()=="mailto"){openExternal(url);return;}
    if(!url.isLocalFile()){
        QMessageBox::warning(this,"无法打开链接","不支持的链接地址：\n"+url.toDisplayString());return;
    }
    auto path=QDir::cleanPath(url.toLocalFile());QFileInfo info(path);
    if(!info.exists()){
        QMessageBox::warning(this,"本地链接不存在","找不到链接目标：\n"+QDir::toNativeSeparators(path)+"\n\n相对链接以当前 Markdown 文件所在目录为起点。");return;
    }
    auto localUrl=QUrl::fromLocalFile(path);
    if(info.isDir()){
        for(const auto &name:{"README.md","readme.md","index.md","README.markdown"}){auto candidate=QDir(path).filePath(name);if(QFileInfo(candidate).isFile()){openPath(candidate);return;}}
        openExternal(localUrl);return;
    }
    if(isMarkdown(path)||QStringList{"txt","log","ini","cfg","json","yaml","yml","cpp","h","py","csv"}.contains(info.suffix().toLower())){
        if(openPath(path)&&!url.fragment().isEmpty())navigate(QUrl("#"+url.fragment()));
    }else openExternal(localUrl);
}

void EditorWindow::format(const QString &kind) {
    auto p=current();if(!p)return;
    if(!p->activeRich()){
        auto c=p->source->textCursor();auto selected=c.selectedText();
        if(kind=="bold"||kind=="italic"){auto mark=kind=="bold"?"**":"*";c.insertText(mark+selected+mark);}
        else {c.movePosition(QTextCursor::StartOfBlock);c.insertText(kind.startsWith('h')?QString(kind.mid(1).toInt(),'#')+' ':kind=="list"?"- ":"");}
        return;
    }
    auto c=p->visual->textCursor();
    if(kind=="bold"||kind=="italic"){auto f=c.charFormat();if(kind=="bold")f.setFontWeight(f.fontWeight()>=QFont::Bold?QFont::Normal:QFont::Bold);else f.setFontItalic(!f.fontItalic());c.mergeCharFormat(f);p->visual->mergeCurrentCharFormat(f);}
    else if(kind=="list"){QTextListFormat f;f.setStyle(QTextListFormat::ListDisc);c.createList(f);}
    else {auto b=c.blockFormat();const int level=kind.startsWith('h')?kind.mid(1).toInt():0;b.setHeadingLevel(level);c.setBlockFormat(b);QTextCharFormat f;f.setFontPointSize(level==1?30:level==2?23:level==3?17:12);f.setFontWeight(level?QFont::Bold:QFont::Normal);c.select(QTextCursor::BlockUnderCursor);c.mergeCharFormat(f);}
}

void EditorWindow::style() {
    const QString bg=dark?"#202226":"#ffffff", fg=dark?"#d9dce1":"#30343b", side=dark?"#191b1e":"#f5f5f4", line=dark?"#34373d":"#e9e9e7", muted=dark?"#969da8":"#8a8e94";
    setStyleSheet(QString(R"(
        QMainWindow,QWidget { background:%1; color:%2; font-family:'Microsoft YaHei UI'; font-size:10pt; }
        QMenuBar,QToolBar {background:%1; border:0; padding:5px 14px; spacing:9px;}
        QToolButton {border:0;border-radius:5px;padding:6px 12px;} QToolButton:hover,QToolButton:checked {background:%3;}
        QWidget#sidebar,QWidget#sidebar QLabel,QTreeWidget {background:%3;}
        QLabel#brand {font-size:13pt;letter-spacing:3px;font-weight:600;padding-bottom:26px;}
        QLabel#subtitle {color:%5;font-size:9pt;padding-bottom:12px;}
        QTreeWidget {border:0;outline:0;} QTreeWidget::item {padding:9px 0;} QTreeWidget::item:selected {background:%4;color:%2;}
        QTabWidget::pane {border:0;} QTabBar::tab {background:%1;color:%5;padding:11px 24px;border-bottom:2px solid transparent;}
        QTabBar::tab:selected {color:%2;border-bottom:2px solid #7c9db0;}
        QTextEdit#paper {border:0; padding:10px 0; font-size:12pt;selection-background-color:#bdd8eb;selection-color:#20252a;}
        QPlainTextEdit#source {border:0;padding:28px;font-family:Consolas;font-size:12pt;selection-background-color:#bdd8eb;}
        QSplitter::handle {background:%4;width:1px;} QStatusBar {border-top:1px solid %4;padding:5px;color:%5;}
        QScrollBar:vertical {background:%1;width:10px;margin:0;} QScrollBar::handle:vertical {background:%4;min-height:30px;border-radius:5px;} QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical {height:0;}
        QLineEdit {padding:7px;border:1px solid %4;border-radius:4px;} QPushButton {padding:6px 12px;border:1px solid %4;border-radius:4px;} QPushButton:hover {background:%3;}
        QMenu {border:1px solid %4;padding:5px;} QMenu::item {padding:7px 28px;} QMenu::item:selected {background:%3;}
    )").arg(bg,fg,side,line,muted));
}

void EditorWindow::dragEnterEvent(QDragEnterEvent *event) {if(event->mimeData()->hasUrls())event->acceptProposedAction();}
void EditorWindow::dropEvent(QDropEvent *event) {for(const auto &url:event->mimeData()->urls())if(url.isLocalFile())openPath(url.toLocalFile());}

bool EditorWindow::eventFilter(QObject *object,QEvent *event) {
    if(event->type()==QEvent::MouseButtonPress){
        auto mouse=static_cast<QMouseEvent *>(event);auto p=current();pressedLink.clear();
        if(p&&object==p->visual->viewport()&&mouse->button()==Qt::LeftButton){
            pressedLink=p->visual->anchorAt(mouse->position().toPoint());linkPressPosition=mouse->position().toPoint();
        }
    }
    if(event->type()==QEvent::KeyPress){
        auto key=static_cast<QKeyEvent *>(event);auto p=current();
        if(p&&object==p->visual&&isMarkdown(p->file.path)&&key->modifiers()==Qt::NoModifier){
            auto c=p->visual->textCursor();const auto prefix=c.block().text().left(c.positionInBlock());
            if(key->key()==Qt::Key_Space&&!c.hasSelection()&&c.atBlockEnd()){
                static QRegularExpression heading("^#{1,6}$");
                if(heading.match(prefix).hasMatch()||prefix=="-"||prefix=="*"||prefix==">"||prefix=="1."){
                    c.beginEditBlock();c.movePosition(QTextCursor::StartOfBlock,QTextCursor::KeepAnchor);c.removeSelectedText();
                    if(prefix.startsWith('#')){
                        int level=prefix.size();auto b=c.blockFormat();b.setHeadingLevel(level);c.setBlockFormat(b);
                        QTextCharFormat f;f.setFontPointSize(level==1?30:level==2?23:17);f.setFontWeight(QFont::Bold);c.setCharFormat(f);
                    }else if(prefix==">"){
                        auto b=c.blockFormat();b.setIndent(1);b.setProperty(QTextFormat::BlockQuoteLevel,1);c.setBlockFormat(b);
                    }else {QTextListFormat f;f.setStyle(prefix=="1."?QTextListFormat::ListDecimal:QTextListFormat::ListDisc);c.createList(f);}
                    c.endEditBlock();p->visual->setTextCursor(c);return true;
                }
            }
            if((key->key()==Qt::Key_Return||key->key()==Qt::Key_Enter)&&c.blockFormat().headingLevel()&&!c.hasSelection()&&c.atBlockEnd()){
                c.beginEditBlock();c.insertBlock(QTextBlockFormat(),QTextCharFormat());c.endEditBlock();p->visual->setTextCursor(c);return true;
            }
        }
    }
    if(event->type()==QEvent::MouseButtonRelease){
        auto mouse=static_cast<QMouseEvent *>(event);auto p=current();
        if(p&&object==p->visual->viewport()&&mouse->button()==Qt::LeftButton){
            auto link=p->visual->anchorAt(mouse->position().toPoint());
            const bool click=(mouse->position().toPoint()-linkPressPosition).manhattanLength()<QApplication::startDragDistance();
            if(!link.isEmpty()&&link==pressedLink&&click){pressedLink.clear();navigate(QUrl(link));return true;}
        }
        pressedLink.clear();
    }
    return QMainWindow::eventFilter(object,event);
}
