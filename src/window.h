#pragma once
#include "editor.h"
#include "settings.h"
#include <QMainWindow>
#include <QTabWidget>
class QAction;
class QLabel;
class QSplitter;
class SearchPanel;

class Window : public QMainWindow {
public:
    explicit Window(const QString &configDirectory = QString(), bool restoreSession = true);
    void openFile(const QString &path);
protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
private:
    Editor *current() const;
    Editor *newTab();
    void attachEditor(Editor *editor);
    bool openDocument(const QString &path, QString *error);
    void refresh();
    void refreshStatus();
    bool save(Editor *editor, bool saveAs = false);
    bool mayClose(Editor *editor);
    void closeTab(int index);
    void editRules(bool blocks);
    bool setPreferences(const AppSettings &candidate);
    void applyPreferences();
    void updateWrap();
    SessionState sessionState() const;
    void syncSession();
    void warn(const QString &title, const QString &message);
    QTabWidget *tabs;
    QSplitter *splitter;
    SearchPanel *searchPanel;
    QLabel *statistics;
    SettingsStore settingsStore;
    SessionStore sessionStore;
    AppSettings settings;
    std::shared_ptr<const HighlightRules> rules;
    QAction *blocksAction = nullptr, *phrasesAction = nullptr, *findAction = nullptr, *findColorAction = nullptr;
    QAction *dayAction = nullptr, *nightAction = nullptr;
    QVector<QAction *> wrapActions;
    bool restoring = true, closing = false;
    QString lastSessionError;
};
