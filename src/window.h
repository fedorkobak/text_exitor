#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include "editor.h"
class Window : public QMainWindow {
public:
    Window();
    void openFile(const QString &path);
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    Editor *current() const;
    Editor *newTab();
    void refresh();
    bool save(Editor *editor, bool saveAs = false);
    bool mayClose(Editor *editor);
    void closeTab(int index);
    void findText();
    QTabWidget *tabs;
};
