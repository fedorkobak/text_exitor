#pragma once
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QTimer>
#include <tree_sitter/api.h>

class SyntaxHighlighter : public QSyntaxHighlighter {
public:
    explicit SyntaxHighlighter(QTextDocument *document);
    ~SyntaxHighlighter() override;
    void setLanguage(const QString &language);
    void parse();
    int spanCount() const { return spans.size(); }
protected:
    void highlightBlock(const QString &text) override;
private:
    struct Span { int start, end; QColor color; };
    QVector<Span> spans;
    TSParser *parser = nullptr;
    bool enabled = false;
    QTimer timer;
};

class Editor : public QPlainTextEdit {
public:
    explicit Editor(QWidget *parent = nullptr);
    bool load(const QString &file, QString *error);
    bool save(const QString &file, QString *error);
    QString path() const { return filePath; }
    QString language() const { return languageName; }
    void setLanguage(const QString &name);
    SyntaxHighlighter *highlighter;
private:
    void detectLanguage();
    QString filePath, languageName = "Plain text";
    bool bom = false;
    QByteArray newline = "\n";
};
