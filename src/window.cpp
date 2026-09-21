#include "window.h"
#include <QAction>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QToolBar>

Window::Window() : tabs(new QTabWidget(this)) {
    setCentralWidget(tabs);
    tabs->setTabsClosable(true);
    tabs->setMovable(true);
    auto file = menuBar()->addMenu("&File");
    auto toolbar = addToolBar("File");
    auto action = [this](QMenu *menu, const QString &label, const QKeySequence &key, auto callback) {
        auto a = menu->addAction(label);
        a->setShortcut(key);
        connect(a, &QAction::triggered, this, callback);
        return a;
    };
    toolbar->addAction(action(file, "&New", QKeySequence::New, [this] { newTab(); }));
    toolbar->addAction(action(file, "&Open…", QKeySequence::Open, [this] {
        for (const auto &path : QFileDialog::getOpenFileNames(this, "Open files")) openFile(path);
    }));
    toolbar->addAction(action(file, "&Save", QKeySequence::Save, [this] { save(current()); }));
    action(file, "Save &As…", QKeySequence::SaveAs, [this] { save(current(), true); });
    action(file, "&Close tab", QKeySequence::Close, [this] { closeTab(tabs->currentIndex()); });
    file->addSeparator();
    action(file, "E&xit", QKeySequence::Quit, [this] { close(); });
    auto edit = menuBar()->addMenu("&Edit");
    action(edit, "&Undo", QKeySequence::Undo, [this] { current()->undo(); });
    action(edit, "&Redo", QKeySequence::Redo, [this] { current()->redo(); });
    edit->addSeparator();
    action(edit, "Cu&t", QKeySequence::Cut, [this] { current()->cut(); });
    action(edit, "&Copy", QKeySequence::Copy, [this] { current()->copy(); });
    action(edit, "&Paste", QKeySequence::Paste, [this] { current()->paste(); });
    action(edit, "Select &All", QKeySequence::SelectAll, [this] { current()->selectAll(); });
    action(edit, "&Find…", QKeySequence::Find, [this] { findText(); });
    auto language = menuBar()->addMenu("&Language");
    for (const auto &name : QStringList{"Plain text", "C/C++", "JSON"})
        action(language, name, {}, [this, name] { current()->setLanguage(name); refresh(); });
    auto view = menuBar()->addMenu("&View");
    auto wrap = action(view, "Word wrap", {}, [this](bool checked) {
        for (int i = 0; i < tabs->count(); ++i) static_cast<Editor *>(tabs->widget(i))->setLineWrapMode(checked ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    });
    wrap->setCheckable(true);
    wrap->setObjectName("wrapAction");
    connect(tabs, &QTabWidget::tabCloseRequested, this, &Window::closeTab);
    connect(tabs, &QTabWidget::currentChanged, this, [this] { refresh(); });
    resize(1000, 700);
    restoreGeometry(QSettings().value("geometry").toByteArray());
    newTab();
}
Editor *Window::current() const { return static_cast<Editor *>(tabs->currentWidget()); }
Editor *Window::newTab() {
    auto editor = new Editor;
    if (findChild<QAction *>("wrapAction")->isChecked()) editor->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    tabs->addTab(editor, "Untitled");
    tabs->setCurrentWidget(editor);
    connect(editor->document(), &QTextDocument::modificationChanged, this, [this] { refresh(); });
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, [this] { refresh(); });
    editor->setFocus();
    refresh();
    return editor;
}
void Window::refresh() {
    for (int i = 0; i < tabs->count(); ++i) {
        auto editor = static_cast<Editor *>(tabs->widget(i));
        QString title = editor->path().isEmpty() ? "Untitled" : QFileInfo(editor->path()).fileName();
        if (editor->document()->isModified()) title += " *";
        tabs->setTabText(i, title);
        tabs->setTabToolTip(i, editor->path());
    }
    if (auto editor = current()) {
        setWindowTitle(tabs->tabText(tabs->currentIndex()) + " — Text Editor");
        const auto cursor = editor->textCursor();
        statusBar()->showMessage(QString("Line %1, Column %2    |    UTF-8    |    %3").arg(cursor.blockNumber() + 1).arg(cursor.positionInBlock() + 1).arg(editor->language()));
    }
}
void Window::openFile(const QString &path) {
    for (int i = 0; i < tabs->count(); ++i) {
        auto editor = static_cast<Editor *>(tabs->widget(i));
        if (!editor->path().isEmpty() && QFileInfo(editor->path()).canonicalFilePath() == QFileInfo(path).canonicalFilePath()) { tabs->setCurrentIndex(i); return; }
    }
    auto editor = new Editor;
    QString error;
    if (!editor->load(path, &error)) { delete editor; QMessageBox::warning(this, "Cannot open file", path + "\n" + error); return; }
    auto blank = current();
    bool replace = blank && blank->path().isEmpty() && blank->toPlainText().isEmpty() && !blank->document()->isModified();
    tabs->addTab(editor, QFileInfo(path).fileName());
    tabs->setCurrentWidget(editor);
    if (replace) { tabs->removeTab(tabs->indexOf(blank)); delete blank; }
    if (findChild<QAction *>("wrapAction")->isChecked()) editor->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    connect(editor->document(), &QTextDocument::modificationChanged, this, [this] { refresh(); });
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, [this] { refresh(); });
    editor->setFocus();
    refresh();
}
bool Window::save(Editor *editor, bool saveAs) {
    QString path = editor->path();
    if (path.isEmpty() || saveAs) path = QFileDialog::getSaveFileName(this, "Save file", path);
    if (path.isEmpty()) return false;
    for (int i = 0; i < tabs->count(); ++i) {
        auto other = static_cast<Editor *>(tabs->widget(i));
        if (other != editor && !other->path().isEmpty() && QFileInfo(other->path()).canonicalFilePath() == QFileInfo(path).canonicalFilePath() && QFileInfo(path).exists()) {
            QMessageBox::warning(this, "File already open", "Close the other tab for this file before overwriting it.");
            return false;
        }
    }
    QString error;
    if (!editor->save(path, &error)) { QMessageBox::warning(this, "Cannot save file", error); return false; }
    refresh();
    return true;
}
bool Window::mayClose(Editor *editor) {
    if (!editor->document()->isModified()) return true;
    auto answer = QMessageBox::warning(this, "Unsaved changes", "Save changes to " + (editor->path().isEmpty() ? QString("Untitled") : editor->path()) + "?", QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    return answer == QMessageBox::Discard || (answer == QMessageBox::Save && save(editor));
}
void Window::closeTab(int index) {
    auto editor = static_cast<Editor *>(tabs->widget(index));
    if (!editor || !mayClose(editor)) return;
    tabs->removeTab(index);
    delete editor;
    if (tabs->count() == 0) newTab();
    refresh();
}
void Window::closeEvent(QCloseEvent *event) {
    for (int i = 0; i < tabs->count(); ++i) {
        tabs->setCurrentIndex(i);
        if (!mayClose(current())) { event->ignore(); return; }
    }
    QSettings().setValue("geometry", saveGeometry());
    event->accept();
}
void Window::findText() {
    bool ok;
    QString term = QInputDialog::getText(this, "Find", "Text:", QLineEdit::Normal, current()->textCursor().selectedText(), &ok);
    if (!ok || term.isEmpty()) return;
    auto editor = current();
    const auto original = editor->textCursor();
    if (!editor->find(term)) {
        editor->moveCursor(QTextCursor::Start);
        if (!editor->find(term)) { editor->setTextCursor(original); statusBar()->showMessage("Text not found", 3000); }
    }
}
