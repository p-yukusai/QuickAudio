#pragma once

#include <QAbstractSlider>
#include <QColor>
#include <QWheelEvent>

inline QColor volumeColor(double fraction) {
    fraction = qBound(0.0, fraction, 1.0);
    const QColor low("#f2a6ad"), high("#9fbaf2");
    return QColor(qRound(low.red() + (high.red() - low.red()) * fraction),
                  qRound(low.green() + (high.green() - low.green()) * fraction),
                  qRound(low.blue() + (high.blue() - low.blue()) * fraction));
}

inline void volumeWheel(QAbstractSlider *slider, QWheelEvent *event, int &remainder) {
    const QPoint delta = event->angleDelta();
    remainder += delta.y() != 0 ? delta.y() : delta.x();
    const int steps = remainder / 120;
    remainder %= 120;
    if (steps != 0) {
        const qint64 value = qint64(slider->value()) + (slider->invertedControls() ? -steps : steps);
        slider->setValue(int(qBound(qint64(slider->minimum()), value, qint64(slider->maximum()))));
    }
    event->accept();
}