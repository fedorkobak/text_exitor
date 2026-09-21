#pragma once
#include "settings.h"
#include <QHash>
#include <functional>
#include <memory>

// A shared Aho-Corasick automaton: scanning cost depends on text and matches,
// not text length multiplied by the number of configured phrases.
class PhraseMatcher {
public:
    struct Match { int start, length, rule; };
    explicit PhraseMatcher(const QVector<ColorRule> &rules = {});
    void scan(const QString &text, const std::function<void(Match)> &visit) const;
private:
    struct Node {
        QHash<uint, int> edges;
        QVector<int> rules;
        int failure = 0, output = -1;
    };
    QVector<Node> nodes;
    QVector<int> lengths;
};

class BlockMatcher {
public:
    explicit BlockMatcher(const QVector<ColorRule> &rules = {});
    int match(const QString &text) const;
private:
    struct Node { QHash<ushort, int> edges; int rule = -1; };
    QVector<Node> nodes;
};

struct HighlightRules {
    QVector<ColorRule> blocks, phrases;
    BlockMatcher blockMatcher;
    PhraseMatcher phraseMatcher;
    HighlightRules(const QVector<ColorRule> &b, const QVector<ColorRule> &p)
        : blocks(b), phrases(p), blockMatcher(b), phraseMatcher(p) {}
};

QColor readableForeground(const QColor &background);
QColor readableSyntaxColor(const QColor &preferred, const QColor &background);
