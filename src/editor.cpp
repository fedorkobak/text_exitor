#include "editor.h"
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QSaveFile>
#include <QTextBlock>
#include <algorithm>
#include <limits>

extern "C" const TSLanguage *tree_sitter_cpp();
extern "C" const TSLanguage *tree_sitter_json();

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *doc) : QSyntaxHighlighter(doc), parser(ts_parser_new()) {
    timer.setSingleShot(true);
    timer.setInterval(120);
    connect(doc, &QTextDocument::contentsChange, this, [this](int, int removed, int added) {
        if (removed || added) timer.start();
    });
    connect(&timer, &QTimer::timeout, this, [this] { parse(); });
}
SyntaxHighlighter::~SyntaxHighlighter() { ts_parser_delete(parser); }
void SyntaxHighlighter::setLanguage(const QString &name) {
    const TSLanguage *lang = name == "C/C++" ? tree_sitter_cpp() : name == "JSON" ? tree_sitter_json() : nullptr;
    enabled = lang && ts_parser_set_language(parser, lang);
    parse();
}
void SyntaxHighlighter::parse() {
    timer.stop();
    spans.clear();
    const QString text = document()->toPlainText();
    // Native UTF-16 uses the same offsets as QString and QTextDocument.
    if (enabled && quint64(text.size()) * 2 < std::numeric_limits<uint32_t>::max()) {
        ts_parser_set_timeout_micros(parser, 50000);
        TSTree *tree = ts_parser_parse_string_encoding(parser, nullptr,
            reinterpret_cast<const char *>(text.utf16()), uint32_t(text.size() * 2), TSInputEncodingUTF16);
        if (tree) {
            TSTreeCursor cursor = ts_tree_cursor_new(ts_tree_root_node(tree));
            bool done = false;
            while (!done) {
                const TSNode node = ts_tree_cursor_current_node(&cursor);
                const QByteArray type = ts_node_type(node);
                QColor color;
                if (type.contains("comment")) color = QColor("#618254");
                else if (type.contains("string") || type == "char_literal") color = QColor("#a34b18");
                else if (type.contains("number") || type == "true" || type == "false" || type == "null") color = QColor("#9850a0");
                else if (type.contains("type") && ts_node_child_count(node) == 0) color = QColor("#147d83");
                else if (!ts_node_is_named(node) && !type.isEmpty() && ((type[0] >= 'a' && type[0] <= 'z') || type[0] == '#')) color = QColor("#245ac4");
                if (color.isValid()) spans.push_back({int(ts_node_start_byte(node) / 2), int(ts_node_end_byte(node) / 2), color});
                if (!color.isValid() && ts_tree_cursor_goto_first_child(&cursor)) continue;
                while (!ts_tree_cursor_goto_next_sibling(&cursor)) {
                    if (!ts_tree_cursor_goto_parent(&cursor)) { done = true; break; }
                }
            }
            ts_tree_cursor_delete(&cursor);
            ts_tree_delete(tree);
        } else ts_parser_reset(parser);
    }
    rehighlight();
}
void SyntaxHighlighter::highlightBlock(const QString &text) {
    const int begin = currentBlock().position(), end = begin + text.size();
    auto span = std::lower_bound(spans.cbegin(), spans.cend(), begin,
        [](const Span &s, int position) { return s.end <= position; });
    for (; span != spans.cend() && span->start < end; ++span) {
        int start = std::max(begin, span->start);
        setFormat(start - begin, std::min(end, span->end) - start, span->color);
    }
}
Editor::Editor(QWidget *parent) : QPlainTextEdit(parent) {
    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
    highlighter = new SyntaxHighlighter(document());
}
void Editor::setLanguage(const QString &name) {
    languageName = name;
    highlighter->setLanguage(name);
}
void Editor::detectLanguage() {
    const auto ext = QFileInfo(filePath).suffix().toLower();
    if (QStringList{"c", "h", "cpp", "hpp", "cc", "hh", "cxx", "hxx"}.contains(ext)) setLanguage("C/C++");
    else if (ext == "json") setLanguage("JSON");
    else setLanguage("Plain text");
}
bool Editor::load(const QString &file, QString *error) {
    QFile input(file);
    if (!input.open(QIODevice::ReadOnly)) { *error = input.errorString(); return false; }
    QByteArray bytes = input.readAll();
    if (input.error() != QFileDevice::NoError) { *error = input.errorString(); return false; }
    const bool hasBom = bytes.startsWith("\xef\xbb\xbf");
    if (hasBom) bytes.remove(0, 3);
    const QString decoded = QString::fromUtf8(bytes);
    if (decoded.toUtf8() != bytes || bytes.contains('\0')) {
        *error = "This file is not valid UTF-8 text. Convert it to UTF-8 before opening.";
        return false;
    }
    bom = hasBom;
    newline = bytes.contains("\r\n") ? "\r\n" : bytes.contains('\r') ? "\r" : "\n";
    QString normalized = decoded;
    normalized.replace("\r\n", "\n").replace('\r', '\n');
    setPlainText(normalized);
    filePath = QFileInfo(file).absoluteFilePath();
    document()->setModified(false);
    detectLanguage();
    return true;
}
bool Editor::save(const QString &file, QString *error) {
    QSaveFile output(file);
    if (!output.open(QIODevice::WriteOnly)) { *error = output.errorString(); return false; }
    QByteArray bytes = toPlainText().toUtf8();
    if (newline != "\n") bytes.replace("\n", newline);
    if (bom) bytes.prepend("\xef\xbb\xbf");
    if (output.write(bytes) != bytes.size()) { *error = "Write failed: " + output.errorString(); return false; }
    if (!output.commit()) { *error = "Commit failed: " + output.errorString(); return false; }
    filePath = QFileInfo(file).absoluteFilePath();
    document()->setModified(false);
    detectLanguage();
    return true;
}
