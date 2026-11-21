#pragma once

#include <QDial>
#include <QMouseEvent>
#include <QPainter>

class VolumeDial : public QDial {
public:
    explicit VolumeDial(QWidget *parent = nullptr) : QDial(parent) {
        setWrapping(false);
        setFixedSize(150, 150);
        setCursor(Qt::SizeVerCursor);
        setToolTip(tr("Drag up or down to adjust volume"));
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() != Qt::LeftButton) {
            QDial::mousePressEvent(event);
            return;
        }
        setFocus(Qt::MouseFocusReason);
        dragging = true;
        startY = event->position().y();
        startValue = value();
        setSliderDown(true);
        event->accept();
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (!dragging) {
            QDial::mouseMoveEvent(event);
            return;
        }
        setValue(qBound(minimum(), startValue + qRound((startY - event->position().y())
                                                       * (maximum() - minimum()) / 150.0), maximum()));
        event->accept();
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (dragging && event->button() == Qt::LeftButton) {
            dragging = false;
            setSliderDown(false);
            event->accept();
            return;
        }
        QDial::mouseReleaseEvent(event);
    }

    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QRectF ring = QRectF(rect()).adjusted(13, 13, -13, -13);
        const QColor track = isEnabled() ? QColor("#30304f") : QColor("#25243a");
        const QColor accent = isEnabled() ? QColor("#a78bfa") : QColor("#55516f");
        painter.setPen(QPen(track, 8, Qt::SolidLine, Qt::RoundCap));
        painter.drawArc(ring, 225 * 16, -270 * 16);
        const double fraction = maximum() == minimum() ? 0.0
            : double(value() - minimum()) / (maximum() - minimum());
        painter.setPen(QPen(accent, 8, Qt::SolidLine, Qt::RoundCap));
        painter.drawArc(ring, 225 * 16, -qRound(270 * 16 * fraction));
        painter.setPen(QPen(hasFocus() ? accent : QColor("#444061"), 1.5));
        painter.setBrush(isEnabled() ? QColor("#211f39") : QColor("#1a192a"));
        painter.drawEllipse(ring.adjusted(14, 14, -14, -14));
        painter.setPen(isEnabled() ? QColor("#f1f5f9") : QColor("#718096"));
        QFont font = painter.font();
        font.setPixelSize(25);
        font.setBold(true);
        painter.setFont(font);
        painter.drawText(rect(), Qt::AlignCenter, QString::number(value()));
    }

private:
    bool dragging = false;
    qreal startY = 0;
    int startValue = 0;
};