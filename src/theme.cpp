#include "theme.h"
#include "rules.h"
#include <QApplication>
#include <QPalette>
#include <QStyle>

void Theme::apply(const AppSettings &settings) {
    const bool night = settings.night;
    const ThemeColors &colors = night ? settings.nightColors : settings.dayColors;
    QPalette palette = QApplication::style()->standardPalette();
    const QColor window = colors.window;
    const QColor base = colors.editor;
    const QColor text = readableForeground(base);
    const QColor windowText = readableForeground(window);
    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, windowText);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, window);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, window);
    palette.setColor(QPalette::ButtonText, windowText);
    palette.setColor(QPalette::ToolTipBase, base);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Highlight, colors.selection);
    palette.setColor(QPalette::HighlightedText, readableForeground(colors.selection));
    palette.setColor(QPalette::Link, night ? QColor("#8bb9ff") : QColor("#174aab"));
    for (auto role : {QPalette::Text, QPalette::ButtonText, QPalette::WindowText}) {
        const QColor background = role == QPalette::Text ? base : window;
        const QColor foreground = readableForeground(background);
        palette.setColor(QPalette::Disabled, role, QColor((foreground.red() * 3 + background.red()) / 4,
            (foreground.green() * 3 + background.green()) / 4, (foreground.blue() * 3 + background.blue()) / 4));
    }
    QApplication::setPalette(palette);
}
