#pragma once

#include "volume_control_style.h"

#include <QLinearGradient>
#include <QPainter>
#include <QSlider>
#include <QStyle>
#include <QStyleOptionSlider>

class VolumeSlider : public QSlider {
public:
    explicit VolumeSlider(QWidget *parent = nullptr) : QSlider(Qt::Horizontal, parent) {}

    static QColor colorForFraction(double fraction) {
        return volumeColor(fraction);
    }

protected:
    void wheelEvent(QWheelEvent *event) override {
        volumeWheel(this, event, wheelRemainder);
    }
    void paintEvent(QPaintEvent *) override {
        QStyleOptionSlider option;
        initStyleOption(&option);
        const QRect handle = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, this);
        const qreal left = handle.width() / 2.0;
        const qreal right = width() - handle.width() / 2.0;
        const qreal center = handle.center().x();
        const QRectF track(left, height() / 2.0 - 4, qMax(0.0, right - left), 8);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(isEnabled() ? QColor("#39354f") : QColor("#302d3e"));
        painter.drawRoundedRect(track, 4, 4);
        const double fraction = maximum() == minimum() ? 0.0
            : double(sliderPosition() - minimum()) / (maximum() - minimum());
        const qreal start = option.upsideDown ? right : left;
        const QRectF filled(QPointF(qMin(start, center), track.top()),
                            QPointF(qMax(start, center), track.bottom()));
        if (filled.width() > 0) {
            QLinearGradient gradient(start, 0, center, 0);
            gradient.setColorAt(0, isEnabled() ? colorForFraction(0) : QColor("#49465e"));
            gradient.setColorAt(1, isEnabled() ? colorForFraction(fraction) : QColor("#49465e"));
            painter.setBrush(gradient);
            painter.drawRoundedRect(filled, 4, 4);
        }
        const QColor accent = isEnabled() ? colorForFraction(fraction) : QColor("#66637b");
        painter.setPen(QPen(hasFocus() ? QColor("#eee7fa") : accent, 2));
        painter.setBrush(isEnabled() ? accent.lighter(125) : QColor("#66637b"));
        painter.drawEllipse(QPointF(center, height() / 2.0), 9, 9);
    }

private:
    int wheelRemainder = 0;
};