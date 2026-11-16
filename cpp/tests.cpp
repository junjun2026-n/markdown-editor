#include "document.h"
#include "rendering.h"
#include "editor.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTextBlock>
#include <QDesktopServices>

class UrlReceiver : public QObject {
    Q_OBJECT
public:
    QList<QUrl> opened;
public slots:
    void receive(const QUrl &url) { opened.append(url); }
};

class Tests : public QObject {
    Q_OBJECT
private slots:
    void encodings() {
        QTemporaryDir dir;
        for(const auto &encoding:{QByteArray("UTF-8"),QByteArray("UTF-16LE"),QByteArray("UTF-16BE"),QByteArray("GB18030")}){
            FileDocument original;original.text=QString::fromUtf8("# 中文\n测试");original.encoding=encoding;original.newline="\r\n";original.bom=encoding!="GB18030";
            QByteArray data;QString error;auto path=dir.filePath("test.md");
            QVERIFY2(writeDocument(original,path,data,error),qPrintable(error));
            FileDocument loaded;QVERIFY2(readDocument(path,loaded,error),qPrintable(error));
            QCOMPARE(loaded.text,original.text);QCOMPARE(loaded.encoding,encoding);QCOMPARE(loaded.newline,original.newline);QCOMPARE(loaded.bom,original.bom);
            QByteArray second;QVERIFY(writeDocument(loaded,path,second,error));QCOMPARE(second,data);
        }
    }
    void failedSave() {
        QTemporaryDir dir;QString error;QByteArray data;auto path=dir.filePath("test.md");FileDocument f;f.text="original";
        QVERIFY(writeDocument(f,path,data,error));f.encoding="ISO-8859-1";f.text=QString::fromUtf8("中文");
        QVERIFY(!writeDocument(f,path,data,error));QFile file(path);QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),QByteArray("original"));
    }
    void binary() {
        QTemporaryDir dir;QFile file(dir.filePath("binary"));QVERIFY(file.open(QIODevice::WriteOnly));file.write(QByteArray("a\0b",3));file.close();FileDocument f;QString error;QVERIFY(!readDocument(file.fileName(),f,error));
    }
    void compatibility() {
        QTextDocument document;auto source=QString::fromUtf8("## :clapper: 课程内容大纲\n\n&#x20;[Slides]\\(./00Summary/)\n\n😀 :smile:\n\n`:clapper:`\n\n```md\n:clapper:\n```");
        renderMarkdown(&document,source);
        QVERIFY(document.toPlainText().contains(QString::fromUtf8("🎬 课程内容大纲")));
        QVERIFY(document.toPlainText().contains(QString::fromUtf8("😀 😄")));
        QCOMPARE(document.toPlainText().count(":clapper:"),2);
        QVERIFY(document.toHtml().contains("href=\"./00Summary/\""));
        QCOMPARE(document.begin().blockFormat().headingLevel(),2);
    }
    void modeRoundtripPreservesUntouchedSource() {
        FileDocument file;file.text="## :clapper: title\n\n&#x20;[Slides]\\(./00Summary/)\n\n**bold**\n";
        Page page(file);page.setSourceMode(true);page.setSourceMode(false);
        QCOMPARE(page.markdown(),file.text);QVERIFY(!page.modified);
        page.visual->moveCursor(QTextCursor::End);page.visual->insertPlainText("added");
        QVERIFY(page.modified);page.setSourceMode(true);QVERIFY(page.source->toPlainText().contains("added"));
        auto before=page.source->toPlainText();page.setSourceMode(false);QCOMPARE(page.markdown(),before);
    }
    void sourceAndPlainText() {
        FileDocument file;file.path="notes.txt";file.text="# :clapper: text";Page page(file);
        QCOMPARE(page.visual->toPlainText(),file.text);page.visual->insertPlainText("new ");QVERIFY(page.markdown().contains("new "));QVERIFY(!page.markdown().contains("\\#"));
        FileDocument md;md.text="initial";Page editor(md);editor.setSourceMode(true);editor.source->setPlainText("# changed\n\n:clapper:");editor.setSourceMode(false);
        QCOMPARE(editor.visual->document()->begin().blockFormat().headingLevel(),1);QVERIFY(editor.visual->toPlainText().contains(QString::fromUtf8("🎬")));QCOMPARE(editor.markdown(),QString("# changed\n\n:clapper:"));
    }
    void windowOutlineSearchAndNavigation() {
        QTemporaryDir dir;auto path=dir.filePath("root.md");QDir(dir.path()).mkdir("00Summary");
        QFile readme(dir.filePath("00Summary/README.md"));QVERIFY(readme.open(QIODevice::WriteOnly));readme.write("# Slides\n");readme.close();
        QFile root(path);QVERIFY(root.open(QIODevice::WriteOnly));root.write("# Title\n\n## Section\n\nhello hello\n");root.close();
        EditorWindow window;auto p=window.openPath(path);QVERIFY(p);QCOMPARE(window.openPath(path),p);QCOMPARE(window.tabs->count(),1);window.refreshOutline();QCOMPARE(window.outline->topLevelItemCount(),1);QCOMPARE(window.outline->topLevelItem(0)->childCount(),1);
        window.query->setText("hello");window.replacement->setText("hello world");QCOMPARE(window.replaceAll(),2);p->visual->undo();QCOMPARE(p->visual->toPlainText().count("hello"),2);QVERIFY(!p->visual->toPlainText().contains("world"));
        QVERIFY(window.findNext());QVERIFY(window.save(p));
        window.navigate(QUrl("./00Summary/"));QCOMPARE(window.tabs->count(),2);QCOMPARE(QFileInfo(window.current()->file.path).fileName(),QString("README.md"));
    }
    void liveHeadingAndList() {
        EditorWindow window;auto p=window.newPage();
        QTest::keyClicks(p->visual,"## ");
        QCOMPARE(p->visual->textCursor().blockFormat().headingLevel(),2);
        QTest::keyClicks(p->visual,"Heading");QTest::keyClick(p->visual,Qt::Key_Return);
        QCOMPARE(p->visual->textCursor().blockFormat().headingLevel(),0);
        QTest::keyClicks(p->visual,"- ");
        QVERIFY(p->visual->textCursor().currentList());
        QVERIFY(p->markdown().contains("## Heading"));
        p->modified=false;
    }
    void localLinks_data() {
        QTest::addColumn<QString>("link");
        QTest::newRow("relative") << "./child.md";
        QTest::newRow("parent") << "../child.md";
        QTest::newRow("chinese spaces") << QString::fromUtf8("./课程 说明.md");
        QTest::newRow("encoded spaces") << "./course%20notes.md";
        QTest::newRow("windows separator") << ".\\child.md";
        QTest::newRow("absolute drive") << "absolute-drive";
        QTest::newRow("file url") << "file-url";
        QTest::newRow("directory README") << "./summary/";
    }
    void localLinks() {
        QFETCH(QString,link);
        QTemporaryDir dir;QDir(dir.path()).mkdir("docs");
        auto root=dir.filePath("docs/root.md");QFile file(root);QVERIFY(file.open(QIODevice::WriteOnly));file.write("# Root");file.close();
        QString target;
        if(link=="absolute-drive"||link=="file-url")target=dir.filePath("child.md");
        else if(link=="./summary/"){QDir(dir.filePath("docs")).mkdir("summary");target=dir.filePath("docs/summary/README.md");}
        else {auto relative=QUrl(link).path();relative.replace('\\','/');target=QDir(dir.filePath("docs")).absoluteFilePath(relative);}
        QFile child(target);QVERIFY(child.open(QIODevice::WriteOnly));child.write("# Target");child.close();
        EditorWindow window;QVERIFY(window.openPath(root));
        if(link=="absolute-drive")link=QDir::toNativeSeparators(target);
        if(link=="file-url")link=QUrl::fromLocalFile(target).toString();
        window.navigate(QUrl(link));QCOMPARE(QFileInfo(window.current()->file.path).canonicalFilePath(),QFileInfo(target).canonicalFilePath());
    }
    void localOtherFilesUseDefaultApplication() {
        QTemporaryDir dir;UrlReceiver receiver;QDesktopServices::setUrlHandler("file",&receiver,"receive");
        EditorWindow window;auto root=dir.filePath("root.md");
        QFile file(root);QVERIFY(file.open(QIODevice::WriteOnly));file.write("# Root");file.close();QVERIFY(window.openPath(root));
        for(const auto &name:{"slides.pdf","page.html","photo.png","report.docx"}){
            QFile target(dir.filePath(name));QVERIFY(target.open(QIODevice::WriteOnly));target.write("test");target.close();
            window.navigate(QUrl(QString("./")+name));
            QCOMPARE(receiver.opened.last().toLocalFile(),target.fileName());
        }
        QDir(dir.path()).mkdir("empty");window.navigate(QUrl("./empty/"));
        QCOMPARE(QDir::cleanPath(receiver.opened.last().toLocalFile()),dir.filePath("empty"));
        QCOMPARE(receiver.opened.size(),5);QCOMPARE(window.tabs->count(),1);
        QDesktopServices::unsetUrlHandler("file");
    }
    void actualClickOpensStandardAndEscapedLinks() {
        QTemporaryDir dir;auto root=dir.filePath("root.md");
        QFile file(root);QVERIFY(file.open(QIODevice::WriteOnly));file.write("[Child](./child.md)\n\n[Slides]\\(./summary/)\n");file.close();
        QFile child(dir.filePath("child.md"));QVERIFY(child.open(QIODevice::WriteOnly));child.write("# Child");child.close();
        QDir(dir.path()).mkdir("summary");QFile summary(dir.filePath("summary/README.md"));QVERIFY(summary.open(QIODevice::WriteOnly));summary.write("# Summary");summary.close();
        EditorWindow window;auto p=window.openPath(root);window.show();QTest::qWait(30);
        auto clickLabel=[&](const QString &label){
            window.tabs->setCurrentWidget(p);QApplication::processEvents();
            auto cursor=p->visual->document()->find(label);QVERIFY(!cursor.isNull());
            cursor.setPosition(cursor.selectionStart()+1);auto point=p->visual->cursorRect(cursor).center();
            QVERIFY(!p->visual->anchorAt(point).isEmpty());
            QTest::mouseClick(p->visual->viewport(),Qt::LeftButton,Qt::NoModifier,point);
        };
        clickLabel("Child");QCOMPARE(QFileInfo(window.current()->file.path).fileName(),QString("child.md"));
        clickLabel("Slides");QCOMPARE(QFileInfo(window.current()->file.path).fileName(),QString("README.md"));
        QVERIFY(!p->modified);
    }
};
QTEST_MAIN(Tests)
#include "tests.moc"
