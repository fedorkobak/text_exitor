#pragma once
#include <QColor>
#include <QFont>
#include <QStringList>
#include <QVector>

struct ColorRule {
    QString text;
    QColor color;
    bool operator==(const ColorRule &other) const { return text == other.text && color == other.color; }
};
struct ThemeColors {
    QColor editor, window, selection;
    bool operator==(const ThemeColors &other) const { return editor == other.editor && window == other.window && selection == other.selection; }
};

struct AppSettings {
    QVector<ColorRule> blocks, phrases;
    QColor findColor = Qt::red;
    QFont editorFont;
    bool blocksEnabled = true, phrasesEnabled = true, findEnabled = true;
    bool night = false;
    ThemeColors dayColors{QColor("#ffffff"), QColor("#f2f3f5"), QColor("#315fb4")};
    ThemeColors nightColors{QColor("#191c21"), QColor("#292d33"), QColor("#315fb4")};
    QString wrapMode = "automatic";
    QByteArray geometry, splitter;
    AppSettings();
};

// An explicit directory also allows tests to avoid the user's real settings.
class SettingsStore {
public:
    explicit SettingsStore(const QString &directory = QString());
    AppSettings load(QStringList *warnings);
    bool save(const AppSettings &settings, QString *error) const;
    QString directory() const { return configDirectory; }
    QString fileName() const;
private:
    QString configDirectory;
    bool writable = true;
};

struct SessionState { QStringList files; QString active; };
class SessionStore {
public:
    explicit SessionStore(const QString &directory) : configDirectory(directory) {}
    SessionState load(QStringList *warnings);
    bool save(const SessionState &state, QString *error) const;
    QString fileName() const;
private:
    QString configDirectory;
    bool writable = true;
};
