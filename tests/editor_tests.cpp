#include "editor.h"
#include "window.h"
#include "dialogs.h"
#include "searchpanel.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QApplication>
#include <QMessageBox>
#include <QAbstractButton>
#include <QTextBlock>
#include <QTextLayout>
#include <QLineEdit>
#include <QAction>
#include <QSettings>
#include <QDir>
#include <QTableView>
#include <QLabel>

namespace {
QColor backgroundAt(const Editor &editor, int position) {
    const QTextBlock block = editor.document()->findBlock(position);
    for (const auto &range : block.layout()->formats()) {
        if (position - block.position() >= range.start && position - block.position() < range.start + range.length
            && range.format.background().style() != Qt::NoBrush) return range.format.background().color();
    }
    return {};
}
bool writeFile(const QString &path, const QByteArray &text) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(text) == text.size();
}
}

class EditorTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("TextEditorTests");
        QCoreApplication::setApplicationName("TextEditorTests");
    }
    void unicodeAndLineEndings() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString path = dir.filePath("test.json");
        QByteArray bytes = QByteArray("\xef\xbb\xbf") + QString::fromUtf8("{\r\n  \"ключ\": \"😀\"\r\n}\r\n").toUtf8();
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(bytes), qint64(bytes.size()));
        file.close();
        Editor editor;
        QString error;
        QVERIFY2(editor.load(path, &error), qPrintable(error));
        QCOMPARE(editor.language(), QString("JSON"));
        QVERIFY(!editor.document()->isModified());
        QVERIFY(editor.highlighter->spanCount() >= 2);
        QVERIFY2(editor.save(path, &error), qPrintable(error));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), bytes);
        file.close();
        QVERIFY(!editor.changedOnDisk());
        QVERIFY(writeFile(path, "external change"));
        QVERIFY(editor.changedOnDisk());
    }
    void failuresPreserveDocument() {
        Editor editor;
        editor.insertPlainText("keep this text");
        QString error;
        QVERIFY(!editor.load("/nonexistent-editor-test-folder/file.txt", &error));
        QCOMPARE(editor.toPlainText(), QString("keep this text"));
        QVERIFY(!editor.save("/nonexistent-editor-test-folder/file.txt", &error));
        QVERIFY(editor.document()->isModified());
        QTemporaryDir dir;
        QFile file(dir.filePath("binary"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QByteArray::fromHex("ff00"));
        file.close();
        QVERIFY(!editor.load(file.fileName(), &error));
        QCOMPARE(editor.toPlainText(), QString("keep this text"));
    }
    void highlightingTracksUnicodeAndEdits() {
        Editor editor;
        editor.setLanguage("C/C++");
        editor.setPlainText(QString::fromUtf8("/* 😀 */ int answer = 42;"));
        editor.highlighter->parse();
        editor.highlighter->rehighlight();
        QVERIFY2(editor.highlighter->spanCount() >= 3, qPrintable(QString::number(editor.highlighter->spanCount())));
        const int number = editor.toPlainText().indexOf("42");
        bool found = false;
        for (const auto &format : editor.document()->firstBlock().layout()->formats())
            if (format.start == number && format.length == 2) found = true;
        QVERIFY(found);
        editor.selectAll();
        editor.insertPlainText("// changed\nint b = 7;");
        QTest::qWait(180);
        QVERIFY(editor.highlighter->spanCount() >= 3);
        editor.setLanguage("Plain text");
        QCOMPARE(editor.highlighter->spanCount(), 0);
    }
    void tabsAndCancelClose() {
        QTemporaryDir dir;
        QFile file(dir.filePath("sample.txt"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("hello");
        file.close();
        Window window(dir.filePath("config"));
        auto tabs = window.findChild<QTabWidget *>();
        QCOMPARE(tabs->count(), 1);
        window.openFile(file.fileName());
        window.openFile(file.fileName());
        QCOMPARE(tabs->count(), 1);
        auto editor = static_cast<Editor *>(tabs->currentWidget());
        editor->insertPlainText("changed");
        QVERIFY(tabs->tabText(0).endsWith(" *"));
        QTimer::singleShot(0, [] {
            auto dialog = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (dialog) dialog->done(QMessageBox::Cancel);
        });
        QVERIFY(!window.close());
        QCOMPARE(tabs->count(), 1);
        QVERIFY(editor->document()->isModified());
    }
    void blockPhraseAndFindPriority() {
        Editor editor;
        AppSettings settings;
        settings.blocks = {{"NOTE", QColor("#ffdd00")}};
        settings.phrases = {{"important message", QColor("#00ddff")}};
        auto rules = std::make_shared<HighlightRules>(settings.blocks, settings.phrases);
        const QString text = "NOTE: Important Message\ncontinued\n\nnormal Important Message";
        editor.setPlainText(text);
        editor.highlighter->configure(rules, settings);
        editor.highlighter->setSearch("MESSAGE");
        editor.highlighter->rehighlight();
        QCOMPARE(backgroundAt(editor, 0), settings.blocks[0].color);
        QCOMPARE(backgroundAt(editor, text.indexOf("continued")), settings.blocks[0].color);
        QCOMPARE(backgroundAt(editor, text.indexOf("Important")), settings.phrases[0].color);
        QCOMPARE(backgroundAt(editor, text.indexOf("Message")), QColor(Qt::red));
        QVERIFY(!backgroundAt(editor, text.indexOf("normal")).isValid());
        settings.findEnabled = false;
        editor.highlighter->configure(rules, settings);
        editor.highlighter->rehighlight();
        QCOMPARE(backgroundAt(editor, text.indexOf("Message")), settings.phrases[0].color);
        settings.phrasesEnabled = false;
        editor.highlighter->configure(rules, settings);
        editor.highlighter->rehighlight();
        QCOMPARE(backgroundAt(editor, text.indexOf("Message")), settings.blocks[0].color);
        settings.blocksEnabled = false;
        editor.highlighter->configure(rules, settings);
        editor.highlighter->rehighlight();
        QVERIFY(!backgroundAt(editor, 0).isValid());
        QCOMPARE(editor.toPlainText(), text);
        QVERIFY(!editor.document()->isModified());
    }
    void blocksPropagateThroughEditsAndUndo() {
        Editor editor;
        AppSettings settings;
        settings.blocks = {{"N", QColor(Qt::yellow)}, {"NOTE", QColor(Qt::green)}};
        editor.highlighter->configure(std::make_shared<HighlightRules>(settings.blocks, settings.phrases), settings);
        editor.setPlainText("NOTE first\nsecond\n\nnormal\nNOTE inside uncolored paragraph");
        editor.highlighter->rehighlight();
        QCOMPARE(backgroundAt(editor, 0), QColor(Qt::green));
        QCOMPARE(backgroundAt(editor, 11), QColor(Qt::green));
        QVERIFY(!backgroundAt(editor, editor.toPlainText().lastIndexOf("NOTE")).isValid());
        QTextCursor cursor = editor.textCursor();
        cursor.setPosition(0);
        cursor.setPosition(4, QTextCursor::KeepAnchor);
        cursor.insertText("other");
        QTRY_VERIFY(!backgroundAt(editor, editor.toPlainText().indexOf("second")).isValid());
        editor.undo();
        QTRY_COMPARE(backgroundAt(editor, 0), QColor(Qt::green));
        cursor = editor.textCursor();
        cursor.setPosition(editor.toPlainText().indexOf("\n\n") + 1);
        cursor.deleteChar();
        QTRY_COMPARE(backgroundAt(editor, editor.toPlainText().indexOf("normal")), QColor(Qt::green));
    }
    void insertingSeparatorActivatesFollowingBlock_data() {
        QTest::addColumn<QString>("separatorText");
        QTest::newRow("empty line") << QString();
        QTest::newRow("whitespace-only line") << QString(" \t");
    }
    void insertingSeparatorActivatesFollowingBlock() {
        QFETCH(QString, separatorText);
        Editor editor;
        AppSettings settings;
        const QColor color("#9141ac");
        settings.blocks = {{"hello", color}};
        editor.highlighter->configure(std::make_shared<HighlightRules>(settings.blocks, settings.phrases), settings);
        const QString original = "this is some file\n\nhello this\nis block color\n\nthis\nhello this\nis not block color\n\nplain tail";
        editor.setPlainText(original);
        // Finish initial/configuration highlighting before exercising the edit;
        // a pending full refresh would conceal an incremental-update failure.
        editor.highlighter->rehighlight();
        QCoreApplication::processEvents();
        const int firstHello = original.indexOf("hello");
        const int secondHello = original.lastIndexOf("hello");
        QCOMPARE(backgroundAt(editor, firstHello), color);
        QVERIFY(!backgroundAt(editor, secondHello).isValid());
        QVERIFY(!backgroundAt(editor, original.indexOf("is not block color")).isValid());

        // Press Enter at the END of "this", rather than at the start of
        // "hello". Qt preserves the following paragraph and its cached state.
        QTextCursor cursor = editor.textCursor();
        cursor.setPosition(secondHello - 1);
        cursor.beginEditBlock();
        cursor.insertBlock();
        cursor.insertText(separatorText);
        cursor.endEditBlock();
        const QString changed = original.left(secondHello - 1) + '\n' + separatorText + original.mid(secondHello - 1);
        QCOMPARE(editor.toPlainText(), changed);
        QTRY_COMPARE(backgroundAt(editor, changed.lastIndexOf("hello")), color);
        QCOMPARE(backgroundAt(editor, changed.indexOf("is not block color")), color);
        QCOMPARE(backgroundAt(editor, firstHello), color);
        QVERIFY(!backgroundAt(editor, changed.indexOf("plain tail")).isValid());

        editor.undo();
        QCOMPARE(editor.toPlainText(), original);
        QTRY_VERIFY(!backgroundAt(editor, secondHello).isValid());
        QVERIFY(!backgroundAt(editor, original.indexOf("is not block color")).isValid());
        QCOMPARE(backgroundAt(editor, firstHello), color);
        editor.redo();
        QCOMPARE(editor.toPlainText(), changed);
        QTRY_COMPARE(backgroundAt(editor, changed.lastIndexOf("hello")), color);
        QCOMPARE(backgroundAt(editor, changed.indexOf("is not block color")), color);
    }
    void unicodeOverlapsAndLargeRuleSet() {
        QVector<ColorRule> rules{{"aba", Qt::yellow}, {"ba", Qt::green}, {QString::fromUtf8("ВАЖНО 😀"), Qt::cyan}};
        for (int i = 0; i < 5000; ++i) rules.append({QString("unused phrase %1!").arg(i), Qt::blue});
        PhraseMatcher matcher(rules);
        QVector<PhraseMatcher::Match> matches;
        matcher.scan(QString::fromUtf8("ababa важно 😀"), [&matches](PhraseMatcher::Match match) { matches.append(match); });
        QCOMPARE(matches.size(), 5);
        QCOMPARE(matches.last().start, 6);
        QCOMPARE(matches.last().length, 8);
        Editor editor;
        AppSettings settings;
        settings.phrases = rules;
        editor.highlighter->configure(std::make_shared<HighlightRules>(settings.blocks, settings.phrases), settings);
        editor.setPlainText("ababa");
        editor.highlighter->rehighlight();
        QCOMPARE(backgroundAt(editor, 0), QColor(Qt::yellow));
        for (int i = 1; i < 5; ++i) QCOMPARE(backgroundAt(editor, i), QColor(Qt::green));
        BlockMatcher prefixes({{"N", Qt::yellow}, {"NOTE", Qt::green}, {"NOTE", Qt::red}, {"", Qt::blue}});
        QCOMPARE(prefixes.match("NOTE text"), 2);
        QCOMPARE(prefixes.match("note text"), -1);
    }
    void unicodeStatisticsAfterReplacementAndUndo() {
        Editor editor;
        editor.setPlainText(QString::fromUtf8("A😀\nБ"));
        QCOMPARE(editor.characterTotal(), qint64(4));
        editor.moveCursor(QTextCursor::End);
        QCOMPARE(editor.cursorColumn(), 2);
        editor.selectAll();
        editor.insertPlainText(QString::fromUtf8("😀😀"));
        QCOMPARE(editor.characterTotal(), qint64(2));
        QCOMPARE(editor.cursorColumn(), 3);
        editor.undo();
        QCOMPARE(editor.characterTotal(), qint64(4));
        editor.redo();
        QCOMPARE(editor.characterTotal(), qint64(2));
        editor.clear();
        QCOMPARE(editor.characterTotal(), qint64(0));
        editor.setPlainText(QString(100000, QChar('x')));
        QCOMPARE(editor.characterTotal(), qint64(100000));
    }
    void settingsAndSessionRoundTrip() {
        QTemporaryDir dir;
        SettingsStore store(dir.path());
        QStringList warnings;
        AppSettings settings = store.load(&warnings);
        QVERIFY(warnings.isEmpty());
        QCOMPARE(settings.findColor, QColor(Qt::red));
        settings.blocks = {{QString::fromUtf8("ВАЖНО"), QColor("#00aa77")}};
        settings.phrases = {{"Important Message", QColor("#aabbcc")}};
        settings.night = true;
        settings.findEnabled = false;
        settings.editorFont.setPointSizeF(28);
        settings.wrapMode = "on";
        settings.nightColors.editor = QColor("#202040");
        QString error;
        QVERIFY2(store.save(settings, &error), qPrintable(error));
        const AppSettings restored = store.load(&warnings);
        QVERIFY(restored.blocks == settings.blocks);
        QVERIFY(restored.phrases == settings.phrases);
        QVERIFY(restored.night);
        QVERIFY(!restored.findEnabled);
        QCOMPARE(restored.editorFont.pointSizeF(), 28.0);
        QCOMPARE(restored.wrapMode, QString("on"));
        QVERIFY(restored.nightColors == settings.nightColors);
        SessionStore session(dir.path());
        SessionState state{{"/a.txt", "/b.txt"}, "/b.txt"};
        QVERIFY2(session.save(state, &error), qPrintable(error));
        QCOMPARE(session.load(&warnings).files, state.files);
        QCOMPARE(session.load(&warnings).active, state.active);
        QVERIFY(QFileInfo::exists(dir.filePath("op_doc.ini")));
        QVERIFY(QFileInfo::exists(dir.filePath("settings.ini")));
    }
    void invalidSettingsAndWriteFailure() {
        QTemporaryDir dir;
        QVERIFY(writeFile(dir.filePath("settings.ini"), "[highlight]\nfindColor=not-a-color\nblocks=garbage\n[phrases]\nsize=2147483647\n1\\text=\n1\\color=bad\n"));
        SettingsStore store(dir.path());
        QStringList warnings;
        const AppSettings settings = store.load(&warnings);
        QCOMPARE(settings.findColor, QColor(Qt::red));
        QVERIFY(settings.blocksEnabled);
        QVERIFY(settings.phrases.isEmpty());
        QVERIFY(!warnings.isEmpty());
        const QString impossible = dir.filePath("regular-file");
        QVERIFY(writeFile(impossible, "keep"));
        SettingsStore blocked(impossible);
        QString error;
        QVERIFY(!blocked.save(settings, &error));
        QVERIFY(!error.isEmpty());
        QFile original(impossible);
        QVERIFY(original.open(QIODevice::ReadOnly));
        QCOMPARE(original.readAll(), QByteArray("keep"));
    }
    void restoreValidDocumentsAndPruneMissing() {
        QTemporaryDir dir;
        const QString first = dir.filePath("first.txt"), second = dir.filePath("second.txt");
        QVERIFY(writeFile(first, "first"));
        QVERIFY(writeFile(second, "second"));
        SessionStore session(dir.filePath("config"));
        QString error;
        QVERIFY(session.save({{first, dir.filePath("missing.txt"), second, first}, first}, &error));
        Window window(dir.filePath("config"));
        const auto tabs = window.findChild<QTabWidget *>();
        QCOMPARE(tabs->count(), 2);
        QCOMPARE(static_cast<Editor *>(tabs->currentWidget())->path(), TextFile::identity(first));
        QStringList warnings;
        QCOMPARE(session.load(&warnings).files, (QStringList{first, second}));
        window.openFile(first);
        QCOMPARE(tabs->count(), 2);
        for (auto box : window.findChildren<QMessageBox *>()) box->accept();
        QVERIFY(window.close());
        Window restored(dir.filePath("config"));
        QCOMPARE(restored.findChild<QTabWidget *>()->count(), 2);
    }
    void persistentSearchAndHighlightToggles() {
        QTemporaryDir dir;
        Window window(dir.path());
        auto editor = static_cast<Editor *>(window.findChild<QTabWidget *>()->currentWidget());
        auto search = window.findChild<QLineEdit *>("findText");
        QVERIFY(search);
        editor->setPlainText("abc ABC abc");
        search->setText("AbC");
        QTRY_COMPARE(backgroundAt(*editor, 4), QColor(Qt::red));
        window.findChild<QAction *>("findEnabled")->trigger();
        QTRY_VERIFY(!backgroundAt(*editor, 4).isValid());
        QVERIFY(!editor->document()->isModified());
        QStringList warnings;
        QVERIFY(!SettingsStore(dir.path()).load(&warnings).findEnabled);
        SearchPanel *panel = window.findChild<SearchPanel *>();
        editor->moveCursor(QTextCursor::Start);
        panel->navigate(editor, false);
        QCOMPARE(editor->textCursor().selectionStart(), 0);
        panel->navigate(editor, false);
        QCOMPARE(editor->textCursor().selectionStart(), 4);
        panel->navigate(editor, true);
        QCOMPARE(editor->textCursor().selectionStart(), 0);
        panel->navigate(editor, true);
        QCOMPARE(editor->textCursor().selectionStart(), 8);
    }
    void coloringDialogDiscardAndSave() {
        QTemporaryDir dir;
        const QVector<ColorRule> original{{"NOTE", Qt::yellow}};
        RulesDialog dialog("Blocks Color", original, true, nullptr);
        auto model = dialog.findChild<QTableView *>()->model();
        QVERIFY(model->setData(model->index(0, 0), "CHANGED"));
        QTimer discard;
        discard.setInterval(10);
        connect(&discard, &QTimer::timeout, [&discard] {
            auto box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (box && box->button(QMessageBox::Discard)) {
                discard.stop();
                box->button(QMessageBox::Discard)->click();
            }
        });
        discard.start();
        dialog.reject();
        QCOMPARE(dialog.result(), int(QDialog::Rejected));
        RulesDialog saved("Blocks Color", original, true, nullptr);
        model = saved.findChild<QTableView *>()->model();
        QVERIFY(model->setData(model->index(0, 0), "CHANGED"));
        QTimer save;
        save.setInterval(10);
        connect(&save, &QTimer::timeout, [&save] {
            auto box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (box && box->button(QMessageBox::Save)) {
                save.stop();
                box->button(QMessageBox::Save)->click();
            }
        });
        save.start();
        saved.reject();
        QCOMPARE(saved.result(), int(QDialog::Accepted));
        QCOMPARE(saved.rules().first().text, QString("CHANGED"));
    }
};
QTEST_MAIN(EditorTests)
#include "editor_tests.moc"
