#include "settings.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QUuid>
#include <algorithm>
#include <functional>

namespace {
void prepare(QSettings &ini) {
    ini.setFallbacksEnabled(false);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    ini.setIniCodec("UTF-8");
#endif
}
bool preserveInvalid(QSettings &ini, QStringList *warnings) {
    if (ini.status() == QSettings::NoError) return true;
    const QString backup = ini.fileName() + ".invalid-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    const bool copied = QFile::copy(ini.fileName(), backup);
    warnings->append(copied
        ? QString("Some settings could not be read. A copy was preserved at %1. Valid entries will still be used.").arg(backup)
        : QString("Cannot read or back up %1. Automatic writes to this file are disabled for this run.").arg(ini.fileName()));
    return copied;
}
bool atomicIni(const QString &path, const std::function<void(QSettings &)> &write, QString *error) {
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        *error = "Cannot create the configuration directory for " + path;
        return false;
    }
    QTemporaryFile staging(QFileInfo(path).absolutePath() + "/.settings-XXXXXX.ini");
    if (!staging.open()) { *error = staging.errorString(); return false; }
    const QString temporaryPath = staging.fileName();
    staging.close();
    {
        QSettings ini(temporaryPath, QSettings::IniFormat);
        prepare(ini);
        write(ini);
        ini.sync();
        if (ini.status() != QSettings::NoError) { *error = "Cannot write temporary configuration for " + path; return false; }
    }
    QFile input(temporaryPath);
    if (!input.open(QIODevice::ReadOnly)) { *error = input.errorString(); return false; }
    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly)) { *error = output.errorString(); return false; }
    while (!input.atEnd()) {
        const QByteArray bytes = input.read(65536);
        if (input.error() != QFileDevice::NoError || output.write(bytes) != bytes.size()) {
            *error = "Could not copy configuration to " + path;
            return false;
        }
    }
    if (!output.commit()) { *error = "Cannot replace " + path + ": " + output.errorString(); return false; }
    return true;
}
QVector<ColorRule> readRules(QSettings &ini, const QString &group, QStringList *warnings) {
    QVector<ColorRule> rules;
    ini.beginGroup(group);
    // Read actual entries rather than trusting a potentially corrupt array size.
    QStringList keys = ini.childGroups();
    std::sort(keys.begin(), keys.end(), [](const QString &a, const QString &b) { return a.toInt() < b.toInt(); });
    bool invalid = false;
    for (const QString &key : keys) {
        ini.beginGroup(key);
        ColorRule rule{ini.value("text").toString(), QColor(ini.value("color").toString())};
        if (!rule.text.isEmpty() && !rule.text.contains('\n') && !rule.text.contains('\r')
            && !rule.text.contains(QChar(0x2028)) && !rule.text.contains(QChar(0x2029)) && rule.color.isValid()) {
            rule.color.setAlpha(255);
            rules.append(rule);
        } else invalid = true;
        ini.endGroup();
    }
    ini.endGroup();
    if (invalid) warnings->append("Ignored empty or invalid rules in " + group + ".");
    return rules;
}
void writeRules(QSettings &ini, const QString &group, const QVector<ColorRule> &rules) {
    ini.beginWriteArray(group, rules.size());
    for (int i = 0; i < rules.size(); ++i) {
        ini.setArrayIndex(i);
        ini.setValue("text", rules[i].text);
        ini.setValue("color", rules[i].color.name());
    }
    ini.endArray();
}
bool readBool(QSettings &ini, const QString &key, bool fallback, QStringList *warnings) {
    if (!ini.contains(key)) return fallback;
    const QString value = ini.value(key).toString().toLower();
    if (value == "true" || value == "1") return true;
    if (value == "false" || value == "0") return false;
    warnings->append("Invalid " + key + "; using its default.");
    return fallback;
}
void readTheme(QSettings &ini, const QString &group, ThemeColors &colors, QStringList *warnings) {
    auto read = [&](const QString &name, QColor &target) {
        const QString key = group + '/' + name;
        if (!ini.contains(key)) return;
        const QColor color(ini.value(key).toString());
        if (color.isValid()) { target = color; target.setAlpha(255); }
        else warnings->append("Invalid " + key + "; using the default color.");
    };
    read("editor", colors.editor);
    read("window", colors.window);
    read("selection", colors.selection);
}
void writeTheme(QSettings &ini, const QString &group, const ThemeColors &colors) {
    ini.setValue(group + "/editor", colors.editor.name());
    ini.setValue(group + "/window", colors.window.name());
    ini.setValue(group + "/selection", colors.selection.name());
}
}

AppSettings::AppSettings() : editorFont(QFontDatabase::systemFont(QFontDatabase::FixedFont)) {
    if (editorFont.pointSizeF() <= 0) editorFont.setPointSizeF(11);
}
SettingsStore::SettingsStore(const QString &directory)
    : configDirectory(directory.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) : directory) {}
QString SettingsStore::fileName() const { return QDir(configDirectory).filePath("settings.ini"); }
AppSettings SettingsStore::load(QStringList *warnings) {
    AppSettings settings;
    QSettings ini(fileName(), QSettings::IniFormat);
    prepare(ini);
    settings.blocks = readRules(ini, "blocks", warnings);
    settings.phrases = readRules(ini, "phrases", warnings);
    const QColor find(ini.value("highlight/findColor", "#ff0000").toString());
    if (find.isValid()) { settings.findColor = find; settings.findColor.setAlpha(255); }
    else warnings->append("Invalid Find Color; using red.");
    settings.blocksEnabled = readBool(ini, "highlight/blocks", true, warnings);
    settings.phrasesEnabled = readBool(ini, "highlight/phrases", true, warnings);
    settings.findEnabled = readBool(ini, "highlight/find", true, warnings);
    settings.night = readBool(ini, "appearance/night", false, warnings);
    readTheme(ini, "dayColors", settings.dayColors, warnings);
    readTheme(ini, "nightColors", settings.nightColors, warnings);
    if (ini.contains("appearance/font")) {
        QFont font;
        if (font.fromString(ini.value("appearance/font").toString()) && font.pointSizeF() >= 6 && font.pointSizeF() <= 96)
            settings.editorFont = font;
        else warnings->append("Invalid editor font; using the default font.");
    }
    const QString wrap = ini.value("editor/wrap", "automatic").toString();
    if (QStringList{"automatic", "on", "off"}.contains(wrap)) settings.wrapMode = wrap;
    else warnings->append("Invalid word wrap preference; using automatic wrapping.");
    settings.geometry = ini.value("window/geometry").toByteArray();
    settings.splitter = ini.value("window/splitter").toByteArray();
    writable = preserveInvalid(ini, warnings);
    return settings;
}
bool SettingsStore::save(const AppSettings &s, QString *error) const {
    if (!writable) { *error = "Configuration was unreadable and could not be backed up: " + fileName(); return false; }
    return atomicIni(fileName(), [&s](QSettings &ini) {
        writeRules(ini, "blocks", s.blocks);
        writeRules(ini, "phrases", s.phrases);
        ini.setValue("highlight/findColor", s.findColor.name());
        ini.setValue("highlight/blocks", s.blocksEnabled);
        ini.setValue("highlight/phrases", s.phrasesEnabled);
        ini.setValue("highlight/find", s.findEnabled);
        ini.setValue("appearance/font", s.editorFont.toString());
        ini.setValue("appearance/night", s.night);
        writeTheme(ini, "dayColors", s.dayColors);
        writeTheme(ini, "nightColors", s.nightColors);
        ini.setValue("editor/wrap", s.wrapMode);
        ini.setValue("window/geometry", s.geometry);
        ini.setValue("window/splitter", s.splitter);
    }, error);
}
QString SessionStore::fileName() const { return QDir(configDirectory).filePath("op_doc.ini"); }
SessionState SessionStore::load(QStringList *warnings) {
    QSettings ini(fileName(), QSettings::IniFormat);
    prepare(ini);
    SessionState state{ini.value("documents/files").toStringList(), ini.value("documents/active").toString()};
    state.files.removeAll(QString());
    state.files.removeDuplicates();
    writable = preserveInvalid(ini, warnings);
    return state;
}
bool SessionStore::save(const SessionState &state, QString *error) const {
    if (!writable) { *error = "Session file was unreadable and could not be backed up: " + fileName(); return false; }
    return atomicIni(fileName(), [&state](QSettings &ini) {
        ini.setValue("documents/files", state.files);
        ini.setValue("documents/active", state.active);
    }, error);
}
