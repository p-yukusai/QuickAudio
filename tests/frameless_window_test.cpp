#include <QApplication>
#include <QPushButton>

#include "../src/frameless_window.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    FramelessWindow window;
    window.show();
    app.processEvents();
    if (!(window.windowFlags() & Qt::FramelessWindowHint)) return 1;
    auto *maximize = window.findChild<QPushButton *>("maximizeButton");
    auto *minimize = window.findChild<QPushButton *>("minimizeButton");
    auto *close = window.findChild<QPushButton *>("closeButton");
    if (!maximize || !minimize || !close) return 2;
    maximize->click();
    if (!window.isMaximized() || maximize->accessibleName() != "Restore") return 3;
    maximize->click();
    if (window.isMaximized() || maximize->accessibleName() != "Maximize") return 4;
    minimize->click();
    if (!window.isMinimized()) return 5;
    window.showNormal();
    close->click();
    if (window.isVisible()) return 6;
    return 0;
}