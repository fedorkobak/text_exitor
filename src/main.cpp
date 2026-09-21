#include "window.h"
#include "startup.h"
#include <QApplication>
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setOrganizationName("TextEditor");
    app.setApplicationName("TextEditor");
    app.setApplicationVersion("1.1");
    // Fusion respects the application palette consistently on Qt 5 and Qt 6.
    app.setStyle("Fusion");
    Window window;
    for (int i = 1; i < app.arguments().size(); ++i) window.openFile(app.arguments().at(i));
    window.show();
    checkFontAvailability(&window);
    return app.exec();
}
