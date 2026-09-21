#include "rules.h"
#include <QQueue>
#include <cmath>

namespace {
double luminance(const QColor &color) {
    auto linear = [](double value) { return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4); };
    return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
}
uint nextCodePoint(const QString &text, int &offset) {
    const QChar first = text.at(offset++);
    if (first.isHighSurrogate() && offset < text.size() && text.at(offset).isLowSurrogate())
        return QChar::surrogateToUcs4(first, text.at(offset++));
    return first.unicode();
}
}
PhraseMatcher::PhraseMatcher(const QVector<ColorRule> &rules) : nodes(1) {
    lengths.resize(rules.size());
    for (int rule = 0; rule < rules.size(); ++rule) {
        int node = 0, offset = 0, length = 0;
        while (offset < rules[rule].text.size()) {
            const uint character = QChar::toCaseFolded(nextCodePoint(rules[rule].text, offset));
            int next = nodes[node].edges.value(character, -1);
            if (next < 0) {
                next = nodes.size();
                nodes[node].edges.insert(character, next);
                nodes.append(Node{});
            }
            node = next;
            ++length;
        }
        lengths[rule] = length;
        if (length) nodes[node].rules.append(rule);
    }
    QQueue<int> queue;
    for (int child : nodes[0].edges) queue.enqueue(child);
    while (!queue.isEmpty()) {
        const int node = queue.dequeue();
        // No nodes are appended here, so iterators remain valid.
        for (auto edge = nodes[node].edges.cbegin(); edge != nodes[node].edges.cend(); ++edge) {
            const int child = edge.value();
            int failure = nodes[node].failure;
            while (failure && !nodes[failure].edges.contains(edge.key())) failure = nodes[failure].failure;
            failure = nodes[failure].edges.value(edge.key(), 0);
            nodes[child].failure = failure;
            nodes[child].output = nodes[failure].rules.isEmpty() ? nodes[failure].output : failure;
            queue.enqueue(child);
        }
    }
}
void PhraseMatcher::scan(const QString &text, const std::function<void(Match)> &visit) const {
    if (nodes.size() == 1) return;
    QVector<int> starts;
    starts.reserve(text.size());
    int offset = 0, state = 0;
    while (offset < text.size()) {
        starts.append(offset);
        const uint character = QChar::toCaseFolded(nextCodePoint(text, offset));
        while (state && !nodes[state].edges.contains(character)) state = nodes[state].failure;
        state = nodes[state].edges.value(character, 0);
        for (int match = state; match >= 0; match = nodes[match].output) {
            for (int rule : nodes[match].rules) {
                const int start = starts[starts.size() - lengths[rule]];
                visit({start, offset - start, rule});
            }
        }
    }
}
BlockMatcher::BlockMatcher(const QVector<ColorRule> &rules) : nodes(1) {
    for (int rule = 0; rule < rules.size(); ++rule) {
        if (rules[rule].text.isEmpty()) continue;
        int node = 0;
        for (QChar character : rules[rule].text) {
            int next = nodes[node].edges.value(character.unicode(), -1);
            if (next < 0) {
                next = nodes.size();
                nodes[node].edges.insert(character.unicode(), next);
                nodes.append(Node{});
            }
            node = next;
        }
        nodes[node].rule = rule;
    }
}
int BlockMatcher::match(const QString &text) const {
    int node = 0, rule = -1;
    for (QChar character : text) {
        node = nodes[node].edges.value(character.unicode(), -1);
        if (node < 0) break;
        if (nodes[node].rule >= 0) rule = nodes[node].rule;
    }
    return rule;
}
QColor readableForeground(const QColor &background) {
    return luminance(background) > 0.179 ? QColor(Qt::black) : QColor(Qt::white);
}
QColor readableSyntaxColor(const QColor &preferred, const QColor &background) {
    const double foregroundLight = luminance(preferred), backgroundLight = luminance(background);
    const double contrast = (qMax(foregroundLight, backgroundLight) + 0.05) / (qMin(foregroundLight, backgroundLight) + 0.05);
    return contrast >= 4.5 ? preferred : readableForeground(background);
}
