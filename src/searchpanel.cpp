#include "searchpanel.h"
#include "editor.h"
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBlock>
#include <QVBoxLayout>
#include <limits>

SearchPanel::SearchPanel(QWidget *parent) : QWidget(parent), input(new QLineEdit(this)), message(new QLabel(this)) {
    setMinimumWidth(180);
    auto layout = new QVBoxLayout(this);
    auto label = new QLabel("&Find Text", this);
    label->setBuddy(input);
    layout->addWidget(label);
    input->setObjectName("findText");
    input->setMaxLength(std::numeric_limits<int>::max());
    input->setClearButtonEnabled(true);
    layout->addWidget(input);
    auto previous = new QPushButton("Previous (Shift+F3)", this);
    auto next = new QPushButton("Next (F3)", this);
    layout->addWidget(previous);
    layout->addWidget(next);
    auto hint = new QLabel("Ignores case. All matching text is highlighted when Find Color is enabled.", this);
    hint->setWordWrap(true);
    layout->addWidget(hint);
    message->setWordWrap(true);
    layout->addWidget(message);
    layout->addStretch();
    connect(input, &QLineEdit::textChanged, this, [this](const QString &text) { clearMessage(); emit queryChanged(text); });
    connect(input, &QLineEdit::returnPressed, this, [this] { emit navigationRequested(false); });
    connect(previous, &QPushButton::clicked, this, [this] { emit navigationRequested(true); });
    connect(next, &QPushButton::clicked, this, [this] { emit navigationRequested(false); });
}
QString SearchPanel::query() const { return input->text(); }
void SearchPanel::focusQuery(const QString &selection) {
    if (!selection.isEmpty() && !selection.contains(QChar::ParagraphSeparator)) input->setText(selection);
    input->setFocus();
    input->selectAll();
}
void SearchPanel::clearMessage() { message->clear(); }
void SearchPanel::navigate(Editor *editor, bool backwards) {
    if (!editor || query().isEmpty()) return;
    const QTextCursor original = editor->textCursor();
    int position = original.position();
    if (original.hasSelection()) position = backwards ? original.selectionStart() : original.selectionStart() + 1;
    QTextDocument::FindFlags flags;
    if (backwards) flags |= QTextDocument::FindBackward;
    QTextCursor match = editor->document()->find(query(), position, flags);
    bool wrapped = false;
    if (match.isNull()) {
        wrapped = true;
        match = editor->document()->find(query(), backwards ? editor->document()->characterCount() - 1 : 0, flags);
    }
    if (match.isNull()) { message->setText("Text not found."); return; }
    editor->setTextCursor(match);
    editor->ensureCursorVisible();
    // Keep keyboard focus in the query when navigating with Enter.
    message->setText(wrapped ? "Wrapped around the document." : QString());
}
