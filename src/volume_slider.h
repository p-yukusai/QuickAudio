#pragma once

#include <QLinearGradient>
#include <QPainter>
#include <QSlider>
#include <QStyle>
#include <QStyleOptionSlider>

class VolumeSlider : public QSlider {
public:
    explicit VolumeSlider(QWidget *parent = nullptr) : QSlider(Qt::Horizontal, parent) {}

    static QColor colorForFraction(double fraction) {
        fraction = qBound(0.0, fraction, 1.0);
        const QColor low("#514968"), high("#c9b9e8");
        return QColor(qRound(low.red() + (high.red() - low.red()) * fraction),
                      qRound(low.green() + (high.green() - low.green()) * fraction),
                      qRound(low.blue() + (high.blue() - low.blue()) * fraction));
    }

protected:
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
};