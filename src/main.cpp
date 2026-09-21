#include "window.h"
#include <QApplication>
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setOrganizationName("TextEditor");
    app.setApplicationName("TextEditor");
    Window window;
    for (int i = 1; i < app.arguments().size(); ++i) window.openFile(app.arguments().at(i));
    window.show();
    return app.exec();
}
