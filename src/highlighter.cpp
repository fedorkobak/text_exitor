#include "highlighter.h"
#include <QElapsedTimer>
#include <QTextDocument>
#include <algorithm>

extern "C" const TSLanguage *tree_sitter_cpp();
extern "C" const TSLanguage *tree_sitter_json();

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *doc) : QSyntaxHighlighter(doc), parser(ts_parser_new()) {
    parseTimer.setSingleShot(true);
    parseTimer.setInterval(120);
    refreshTimer.setSingleShot(true);
    connect(doc, &QTextDocument::contentsChange, this, [this](int position, int removed, int added) {
        if (!removed && !added) return;
        // Keep cached syntax positions aligned until the debounced parser runs.
        const int delta = added - removed;
        for (auto &span : spans) {
            if (span.end <= position) continue;
            if (span.start >= position + removed) { span.start += delta; span.end += delta; }
            else { span.start = span.end = position; }
        }
        if (syntaxEnabled) parseTimer.start();
    });
    connect(&parseTimer, &QTimer::timeout, this, [this] { parse(); });
    connect(&refreshTimer, &QTimer::timeout, this, [this] { refreshBatch(); });
}
SyntaxHighlighter::~SyntaxHighlighter() { ts_parser_delete(parser); }
void SyntaxHighlighter::setLanguage(const QString &name) {
    const TSLanguage *language = name == "C/C++" ? tree_sitter_cpp() : name == "JSON" ? tree_sitter_json() : nullptr;
    syntaxEnabled = parser && language && ts_parser_set_language(parser, language);
    parse();
}
void SyntaxHighlighter::configure(std::shared_ptr<const HighlightRules> value, const AppSettings &settings) {
    const QColor background = settings.night ? settings.nightColors.editor : settings.dayColors.editor;
    if (rules == value && blocksEnabled == settings.blocksEnabled && phrasesEnabled == settings.phrasesEnabled
        && findEnabled == settings.findEnabled && findColor == settings.findColor && night == settings.night
        && editorBackground == background) return;
    rules = std::move(value);
    blocksEnabled = settings.blocksEnabled;
    phrasesEnabled = settings.phrasesEnabled;
    findEnabled = settings.findEnabled;
    findColor = settings.findColor;
    night = settings.night;
    editorBackground = background;
    requestRefresh();
}
void SyntaxHighlighter::setSearch(const QString &query) {
    if (searchQuery == query) return;
    searchQuery = query;
    search = std::make_shared<PhraseMatcher>(QVector<ColorRule>{{query, findColor}});
    requestRefresh();
}
void SyntaxHighlighter::requestRefresh() {
    pendingBlock = document()->firstBlock();
    refreshTimer.start(0);
}
void SyntaxHighlighter::refreshBatch() {
    QElapsedTimer budget;
    budget.start();
    while (pendingBlock.isValid()) {
        const QTextBlock block = pendingBlock;
        pendingBlock = pendingBlock.next();
        rehighlightBlock(block);
        if (budget.elapsed() >= 8) break;
    }
    if (pendingBlock.isValid()) refreshTimer.start(0);
}
void SyntaxHighlighter::parse() {
    parseTimer.stop();
    spans.clear();
    // Plain-text editing never copies or parses the whole document.
    // Tree-sitter remains optional; bound its input as well as parsing time.
    if (syntaxEnabled && document()->characterCount() <= 2 * 1024 * 1024) {
        const QString text = document()->toPlainText();
        ts_parser_set_timeout_micros(parser, 50000);
        TSTree *tree = ts_parser_parse_string_encoding(parser, nullptr,
            reinterpret_cast<const char *>(text.utf16()), uint32_t(text.size() * 2), TSInputEncodingUTF16);
        if (tree) {
            TSTreeCursor cursor = ts_tree_cursor_new(ts_tree_root_node(tree));
            QElapsedTimer traversal;
            traversal.start();
            bool done = false;
            int visited = 0;
            while (!done) {
                const TSNode node = ts_tree_cursor_current_node(&cursor);
                const QByteArray type = ts_node_type(node);
                QColor color;
                if (type.contains("comment")) color = QColor("#618254");
                else if (type.contains("string") || type == "char_literal") color = QColor("#a34b18");
                else if (type.contains("number") || type == "true" || type == "false" || type == "null") color = QColor("#9850a0");
                else if (type.contains("type") && ts_node_child_count(node) == 0) color = QColor("#147d83");
                else if (!ts_node_is_named(node) && !type.isEmpty() && ((type[0] >= 'a' && type[0] <= 'z') || type[0] == '#')) color = QColor("#245ac4");
                if (color.isValid()) spans.append({int(ts_node_start_byte(node) / 2), int(ts_node_end_byte(node) / 2), color});
                if (++visited % 1024 == 0 && traversal.elapsed() >= 25) { spans.clear(); break; }
                if (!color.isValid() && ts_tree_cursor_goto_first_child(&cursor)) continue;
                while (!ts_tree_cursor_goto_next_sibling(&cursor)) {
                    if (!ts_tree_cursor_goto_parent(&cursor)) { done = true; break; }
                }
            }
            ts_tree_cursor_delete(&cursor);
            ts_tree_delete(tree);
        } else ts_parser_reset(parser);
    }
    requestRefresh();
}
void SyntaxHighlighter::fill(int start, int length, const QColor &color) {
    QTextCharFormat format;
    format.setBackground(color);
    format.setForeground(readableForeground(color));
    setFormat(start, length, format);
}
void SyntaxHighlighter::highlightBlock(const QString &text) {
    const int begin = currentBlock().position(), end = begin + text.size();
    auto span = std::lower_bound(spans.cbegin(), spans.cend(), begin,
        [](const Span &item, int position) { return item.end <= position; });
    for (; span != spans.cend() && span->start < end; ++span) {
        const int start = std::max(begin, span->start);
        setFormat(start - begin, std::min(end, span->end) - start,
            readableSyntaxColor(night ? span->color.lighter(165) : span->color, editorBackground));
    }
    // State -1: blank separator; 0: uncolored paragraph; n+1: block rule n.
    int state = -1;
    if (!text.trimmed().isEmpty()) {
        state = previousBlockState();
        if (state < 0) state = rules ? rules->blockMatcher.match(text) + 1 : 0;
        if (blocksEnabled && rules && state > 0 && state <= rules->blocks.size())
            fill(0, text.size(), rules->blocks[state - 1].color);
    }
    setCurrentBlockState(state);
    if (phrasesEnabled && rules) {
        QVector<PhraseMatcher::Match> matches;
        rules->phraseMatcher.scan(text, [&matches](PhraseMatcher::Match match) { matches.append(match); });
        // Later rules win overlapping phrase colors, independent of trie order.
        std::sort(matches.begin(), matches.end(), [](const PhraseMatcher::Match &a, const PhraseMatcher::Match &b) { return a.rule < b.rule; });
        for (const auto &match : matches) fill(match.start, match.length, rules->phrases[match.rule].color);
    }
    if (findEnabled && search)
        search->scan(text, [this](PhraseMatcher::Match match) { fill(match.start, match.length, findColor); });
}
