#pragma once
#include "rules.h"
#include <QSyntaxHighlighter>
#include <QTextBlock>
#include <QTimer>
#include <tree_sitter/api.h>

class SyntaxHighlighter : public QSyntaxHighlighter {
public:
    explicit SyntaxHighlighter(QTextDocument *document);
    ~SyntaxHighlighter() override;
    void setLanguage(const QString &language);
    void configure(std::shared_ptr<const HighlightRules> rules, const AppSettings &settings);
    void setSearch(const QString &query);
    void parse();
    int spanCount() const { return spans.size(); }
protected:
    void highlightBlock(const QString &text) override;
private:
    struct Span { int start, end; QColor color; };
    void requestRefresh();
    void refreshBatch();
    void fill(int start, int length, const QColor &color);
    QVector<Span> spans;
    std::shared_ptr<const HighlightRules> rules;
    std::shared_ptr<const PhraseMatcher> search;
    QColor findColor = Qt::red;
    QColor editorBackground = Qt::white;
    QString searchQuery;
    bool blocksEnabled = true, phrasesEnabled = true, findEnabled = true, night = false;
    TSParser *parser = nullptr;
    bool syntaxEnabled = false;
    QTimer parseTimer, refreshTimer;
    QTextBlock pendingBlock;
};
