#include "fileio.h"
#include <QDir>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <limits>

QString TextFile::identity(const QString &path) {
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    return canonical.isEmpty() ? QDir::cleanPath(info.absoluteFilePath()) : canonical;
}
bool TextFile::samePath(const QString &first, const QString &second) {
#ifdef Q_OS_WIN
    return identity(first).compare(identity(second), Qt::CaseInsensitive) == 0;
#else
    return identity(first) == identity(second);
#endif
}
bool TextFile::read(const QString &path, QString *text, TextFileFormat *format, QString *error) {
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile()) { *error = "The file does not exist or is not a regular file."; return false; }
    QFile input(path);
    if (!input.open(QIODevice::ReadOnly)) { *error = input.errorString(); return false; }
    if (input.size() > std::numeric_limits<int>::max() / 2) {
        *error = "This file exceeds the editor's text-buffer capacity (approximately 1 GiB).";
        return false;
    }
    QByteArray bytes = input.readAll();
    if (input.error() != QFileDevice::NoError) { *error = input.errorString(); return false; }
    TextFileFormat candidate;
    candidate.digest = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
    candidate.bom = bytes.startsWith("\xef\xbb\xbf");
    if (candidate.bom) bytes.remove(0, 3);
    QString decoded = QString::fromUtf8(bytes);
    if (decoded.toUtf8() != bytes || bytes.contains('\0')) {
        *error = "This file is not valid UTF-8 text. Convert it to UTF-8 before opening.";
        return false;
    }
    const int crlf = bytes.count("\r\n"), lf = bytes.count('\n') - crlf, cr = bytes.count('\r') - crlf;
    if (crlf > 0 && crlf >= lf && crlf >= cr) candidate.newline = "\r\n";
    else if (cr > lf) candidate.newline = "\r";
    decoded.replace("\r\n", "\n").replace('\r', '\n');
    *text = decoded;
    *format = candidate;
    return true;
}
bool TextFile::changedOnDisk(const QString &path, const QByteArray &digest) {
    if (digest.isEmpty()) return false;
    QFile input(path);
    if (!input.open(QIODevice::ReadOnly)) return true;
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&input)) return true;
    return hash.result() != digest;
}
bool TextFile::write(const QString &path, const QString &text, TextFileFormat *format, QString *error) {
    QSaveFile output(identity(path));
    if (!output.open(QIODevice::WriteOnly)) { *error = output.errorString(); return false; }
    QByteArray bytes = text.toUtf8();
    if (format->newline != "\n") bytes.replace("\n", format->newline);
    if (format->bom) bytes.prepend("\xef\xbb\xbf");
    if (output.write(bytes) != bytes.size()) { *error = "Write failed: " + output.errorString(); return false; }
    if (!output.commit()) { *error = "Commit failed: " + output.errorString(); return false; }
    format->digest = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
    return true;
}
