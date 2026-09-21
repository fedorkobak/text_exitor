#include "startup.h"
#include <QFontDatabase>
#include <QMessageBox>

void checkFontAvailability(QWidget *parent) {
    if (!QFontDatabase().families().isEmpty()) return;
    QString message = "No usable fonts were found. The editor needs installed fonts to display text.\n\n";
#ifdef Q_OS_WIN
    message += "Install or restore fonts through Windows Settings > Personalization > Fonts, then restart the editor.";
#else
    message += "On Ubuntu or Linux Mint, run: sudo apt install fontconfig fonts-dejavu-core\nThen restart the editor.";
#endif
    auto warning = new QMessageBox(QMessageBox::Warning, "Fonts unavailable", message, QMessageBox::Ok, parent);
    warning->setAttribute(Qt::WA_DeleteOnClose);
    warning->open();
}
