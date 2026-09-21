#pragma once
#include <QString>
#include <QByteArray>

struct TextFileFormat { bool bom = false; QByteArray newline = "\n"; QByteArray digest; };
class TextFile {
public:
    static bool read(const QString &path, QString *text, TextFileFormat *format, QString *error);
    static bool write(const QString &path, const QString &text, TextFileFormat *format, QString *error);
    static bool changedOnDisk(const QString &path, const QByteArray &digest);
    static QString identity(const QString &path);
    static bool samePath(const QString &first, const QString &second);
};
