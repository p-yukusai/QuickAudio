#include "../src/volume_dial.h"
#include "../src/volume_slider.h"

#include <QApplication>
#include <QKeyEvent>
#include <QSlider>
#include <QWheelEvent>
#include <cstdlib>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    VolumeDial dial;
    QSlider slider;
    dial.setRange(0, 100);
    slider.setRange(0, 100);
    QObject::connect(&dial, &QDial::valueChanged, &slider, &QSlider::setValue);
    QObject::connect(&slider, &QSlider::valueChanged, &dial, &QDial::setValue);
    auto mouse = [&](QEvent::Type type, int y, Qt::MouseButton button, Qt::MouseButtons buttons) {
        QMouseEvent event(type, QPointF(75, y), button, buttons, Qt::NoModifier);
        QApplication::sendEvent(&dial, &event);
    };
    dial.setValue(100);
    mouse(QEvent::MouseButtonPress, 75, Qt::LeftButton, Qt::LeftButton);
    mouse(QEvent::MouseMove, -100, Qt::NoButton, Qt::LeftButton);
    if (dial.value() != 100 || slider.value() != 100) return EXIT_FAILURE;
    mouse(QEvent::MouseMove, 400, Qt::NoButton, Qt::LeftButton);
    if (dial.value() != 0 || slider.value() != 0) return EXIT_FAILURE;
    mouse(QEvent::MouseButtonRelease, 400, Qt::LeftButton, Qt::NoButton);
    QKeyEvent down(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
    QApplication::sendEvent(&dial, &down);
    if (dial.value() != 0) return EXIT_FAILURE;
    slider.setValue(42);
    if (dial.value() != 42) return EXIT_FAILURE;
    mouse(QEvent::MouseButtonPress, 75, Qt::LeftButton, Qt::LeftButton);
    mouse(QEvent::MouseMove, 74, Qt::NoButton, Qt::LeftButton);
    if (dial.value() != 43) return EXIT_FAILURE;
    mouse(QEvent::MouseButtonRelease, 74, Qt::LeftButton, Qt::NoButton);
    auto wheel = [](QWidget &widget, int delta) {
        QWheelEvent event(QPointF(10, 10), QPointF(10, 10), QPoint(), QPoint(0, delta),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(&widget, &event);
    };
    wheel(dial, 120);
    if (dial.value() != 44) return EXIT_FAILURE;
    wheel(dial, -60);
    if (dial.value() != 44) return EXIT_FAILURE;
    wheel(dial, -60);
    if (dial.value() != 43) return EXIT_FAILURE;
    dial.setValue(100);
    wheel(dial, 120);
    if (dial.value() != 100) return EXIT_FAILURE;
    const QColor low = VolumeSlider::colorForFraction(0);
    const QColor middle = VolumeSlider::colorForFraction(0.5);
    const QColor high = VolumeSlider::colorForFraction(1);
    if (!(low.red() > low.blue() && high.blue() > high.red()
          && middle.red() < low.red() && middle.blue() > low.blue())) return EXIT_FAILURE;
    if (low.lightness() < 150 || middle.lightness() < 150 || high.lightness() < 150) return EXIT_FAILURE;
    if (VolumeSlider::colorForFraction(-1) != low || VolumeSlider::colorForFraction(2) != high) return EXIT_FAILURE;
    VolumeSlider volumeSlider;
    volumeSlider.setRange(0, 100);
    volumeSlider.resize(200, 30);
    volumeSlider.show();
    volumeSlider.setValue(25);
    wheel(volumeSlider, 120);
    if (volumeSlider.value() != 26) return EXIT_FAILURE;
    wheel(volumeSlider, -120);
    if (volumeSlider.value() != 25) return EXIT_FAILURE;
    volumeSlider.setValue(0);
    wheel(volumeSlider, -120);
    if (volumeSlider.value() != 0) return EXIT_FAILURE;
    volumeSlider.setValue(25);
    const QImage dim = volumeSlider.grab().toImage();
    volumeSlider.setValue(75);
    const QImage bright = volumeSlider.grab().toImage();
    if (dim == bright) return EXIT_FAILURE;
    QKeyEvent up(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
    QApplication::sendEvent(&volumeSlider, &up);
    if (volumeSlider.value() != 76) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}