#include "editor.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMimeData>
#include <QTextBlock>
#include <QUrl>
#include <QWheelEvent>

namespace {
int scalarCount(const QString &text) {
    int count = 0;
    for (int i = 0; i < text.size(); ++i) {
        ++count;
        if (text[i].isHighSurrogate() && i + 1 < text.size() && text[i + 1].isLowSurrogate()) ++i;
    }
    return count;
}
struct BlockMetrics : QTextBlockUserData {
    std::shared_ptr<CharacterMetrics> total;
    int count;
    BlockMetrics(std::shared_ptr<CharacterMetrics> value, int size) : total(std::move(value)), count(size) { total->characters += count; }
    ~BlockMetrics() override { total->characters -= count; }
};
}
Editor::Editor(QWidget *parent) : QPlainTextEdit(parent), metrics(std::make_shared<CharacterMetrics>()) {
    setEditorFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    setLineWrapMode(QPlainTextEdit::WidgetWidth);
    connect(document(), &QTextDocument::contentsChange, this, [this](int position, int removed, int added) {
        if (!removed && !added) return;
        updateMetrics(position, added);
        emit statisticsChanged();
    });
    highlighter = new SyntaxHighlighter(document());
}
void Editor::updateMetrics(int position, int added) {
    QTextBlock block = document()->findBlock(qMin(position, document()->characterCount() - 1));
    const int end = position + added;
    while (block.isValid()) {
        const int count = scalarCount(block.text());
        auto data = static_cast<BlockMetrics *>(block.userData());
        if (data) { metrics->characters += count - data->count; data->count = count; }
        else block.setUserData(new BlockMetrics(metrics, count));
        block = block.next();
        if (block.position() > end) break;
    }
}
qint64 Editor::characterTotal() const { return metrics->characters + document()->blockCount() - 1; }
int Editor::cursorColumn() const { return scalarCount(textCursor().block().text().left(textCursor().positionInBlock())) + 1; }
void Editor::setEditorFont(const QFont &value) {
    QFont changed = value;
    changed.setPointSizeF(qBound(6.0, changed.pointSizeF() > 0 ? changed.pointSizeF() : 11.0, 96.0));
    if (font() != changed) setFont(changed);
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
}
void Editor::wheelEvent(QWheelEvent *event) {
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        wheelRemainder += event->angleDelta().y();
        int steps = wheelRemainder / 120;
        wheelRemainder %= 120;
        if (!steps && !event->pixelDelta().isNull()) steps = event->pixelDelta().y() > 0 ? 1 : -1;
        if (steps) {
            QFont changed = font();
            changed.setPointSizeF(qBound(6.0, changed.pointSizeF() + steps, 96.0));
            setEditorFont(changed);
            emit fontZoomed(changed);
        }
        event->accept();
    } else { wheelRemainder = 0; QPlainTextEdit::wheelEvent(event); }
}
void Editor::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
    else QPlainTextEdit::dragEnterEvent(event);
}
void Editor::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
    else QPlainTextEdit::dragMoveEvent(event);
}
void Editor::dropEvent(QDropEvent *event) {
    if (!event->mimeData()->hasUrls()) { QPlainTextEdit::dropEvent(event); return; }
    QStringList paths;
    for (const QUrl &url : event->mimeData()->urls()) paths.append(url.isLocalFile() ? url.toLocalFile() : url.toString());
    emit filesDropped(paths);
    event->acceptProposedAction();
}
void Editor::detectLanguage() {
    const QString ext = QFileInfo(filePath).suffix().toLower();
    if (QStringList{"c", "h", "cpp", "hpp", "cc", "hh", "cxx", "hxx"}.contains(ext)) languageName = "C/C++";
    else if (ext == "json") languageName = "JSON";
    else languageName = "Plain text";
    highlighter->setLanguage(languageName);
}
bool Editor::load(const QString &file, QString *error) {
    QString text;
    TextFileFormat candidate;
    if (!TextFile::read(file, &text, &candidate, error)) return false;
    setPlainText(text);
    updateMetrics(0, document()->characterCount());
    format = candidate;
    filePath = TextFile::identity(file);
    document()->setModified(false);
    detectLanguage();
    emit statisticsChanged();
    return true;
}
bool Editor::save(const QString &file, QString *error) {
    if (!TextFile::write(file, toPlainText(), &format, error)) return false;
    filePath = TextFile::identity(file);
    document()->setModified(false);
    detectLanguage();
    return true;
}
