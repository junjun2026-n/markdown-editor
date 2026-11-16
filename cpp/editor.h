#pragma once
#include "document.h"
#include <QMainWindow>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QSettings>
#include <QTimer>
#include <QAction>

class Page : public QStackedWidget {
public:
    explicit Page(const FileDocument &value, QWidget *parent = nullptr);
    FileDocument file;
    QTextEdit *visual;
    QPlainTextEdit *source;
    bool modified = false;
    bool visualDirty = false;
    bool sourceDirty = false;
    bool updating = false;
    QString markdown();
    void setSourceMode(bool enabled);
    QTextEdit *activeRich() const;
    void syncVisual();
};

class EditorWindow : public QMainWindow {
    Q_OBJECT
public:
    EditorWindow();
    Page *current() const;
    Page *openPath(const QString &path);
    Page *newPage(const QString &text = {});
    bool save(Page *page = nullptr, bool saveAs = false);
    bool confirmClose(Page *page);
    void closeTab(int index);
    void toggleSource(bool enabled);
    void refresh();
    void refreshOutline();
    void navigate(const QUrl &url);
    QTabWidget *tabs;
    QTreeWidget *outline;
    QLineEdit *query;
    QLineEdit *replacement;
    QCheckBox *matchCase;
    QWidget *findBar;
    QAction *sourceAction;
    bool findNext(bool backwards = false);
    int replaceAll();
protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
private:
    QSettings settings;
    QTimer outlineTimer;
    QLabel *info;
    QAction *sidebarAction;
    QAction *focusAction;
    bool dark = false;
    QString pressedLink;
    QPoint linkPressPosition;
    void addPage(Page *page);
    void openFiles();
    void showFind(bool replace);
    void replaceOne();
    void exportHtml();
    void style();
    void format(const QString &kind);
};
