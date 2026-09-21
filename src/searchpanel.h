#pragma once
#include <QWidget>
class QLineEdit;
class QLabel;
class Editor;
class SearchPanel : public QWidget {
    Q_OBJECT
public:
    explicit SearchPanel(QWidget *parent = nullptr);
    QString query() const;
    void focusQuery(const QString &selection = QString());
    void navigate(Editor *editor, bool backwards);
    void clearMessage();
signals:
    void queryChanged(const QString &query);
    void navigationRequested(bool backwards);
private:
    QLineEdit *input;
    QLabel *message;
};
