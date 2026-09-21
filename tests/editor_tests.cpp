#include "editor.h"
#include "window.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QMessageBox>
#include <QTextBlock>
#include <QTextLayout>

class EditorTests : public QObject {
    Q_OBJECT
private slots:
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
        Window window;
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
};
QTEST_MAIN(EditorTests)
#include "editor_tests.moc"
