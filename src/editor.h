#pragma once
#include "fileio.h"
#include "highlighter.h"
#include <QPlainTextEdit>
#include <memory>

struct CharacterMetrics { qint64 characters = 0; };
class Editor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit Editor(QWidget *parent = nullptr);
    bool load(const QString &file, QString *error);
    bool save(const QString &file, QString *error);
    bool changedOnDisk() const { return !filePath.isEmpty() && TextFile::changedOnDisk(filePath, format.digest); }
    QString path() const { return filePath; }
    QString language() const { return languageName; }
    void setEditorFont(const QFont &font);
    qint64 characterTotal() const;
    int cursorColumn() const;
    SyntaxHighlighter *highlighter;
signals:
    void filesDropped(const QStringList &paths);
    void fontZoomed(const QFont &font);
    void statisticsChanged();
protected:
    void wheelEvent(QWheelEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
private:
    void detectLanguage();
    void updateMetrics(int position, int added);
    QString filePath, languageName = "Plain text";
    TextFileFormat format;
    std::shared_ptr<CharacterMetrics> metrics;
    int wheelRemainder = 0;
};
