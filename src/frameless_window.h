#ifndef FRAMELESS_WINDOW_H
#define FRAMELESS_WINDOW_H

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QShortcut>
#include <QWindow>

class FramelessWindow : public QMainWindow {
public:
    FramelessWindow() {
        setWindowFlag(Qt::FramelessWindowHint);
        auto *bar = new QWidget(this);
        bar->setObjectName("titleBar");
        bar->setFixedHeight(40);
        auto *layout = new QHBoxLayout(bar);
        layout->setContentsMargins(12, 0, 0, 0);
        layout->setSpacing(0);
        auto *icon = new QLabel(bar);
        icon->setPixmap(QApplication::windowIcon().pixmap(20, 20));
        icon->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(icon);
        auto *title = new QLabel(tr("QuickAudio"), bar);
        title->setObjectName("windowTitle");
        title->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addSpacing(8);
        layout->addWidget(title, 1);
        auto addButton = [&](const QString &name, const QString &text, const QString &description) {
            auto *button = new QPushButton(text, bar);
            button->setObjectName(name);
            button->setAccessibleName(description);
            button->setToolTip(description);
            button->setFixedSize(46, 40);
            button->setFocusPolicy(Qt::NoFocus);
            layout->addWidget(button);
            return button;
        };
        auto *minimize = addButton("minimizeButton", QString::fromUtf8("\xe2\x88\x92"), tr("Minimize"));
        maximizeButton = addButton("maximizeButton", QString::fromUtf8("\xe2\x96\xa1"), tr("Maximize"));
        auto *close = addButton("closeButton", QString::fromUtf8("\xc3\x97"), tr("Close"));
        connect(minimize, &QPushButton::clicked, this, &QWidget::showMinimized);
        connect(maximizeButton, &QPushButton::clicked, this, [this] { toggleMaximized(); });
        connect(close, &QPushButton::clicked, this, &QWidget::close);
        setMenuWidget(bar);
        titleBar = bar;
        qApp->installEventFilter(this);
        auto *systemMenu = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Space), this);
        connect(systemMenu, &QShortcut::activated, this, [this] { showWindowMenu(mapToGlobal(QPoint(0, titleBar->height()))); });
    }

    ~FramelessWindow() override { qApp->removeEventFilter(this); }

protected:
    void changeEvent(QEvent *event) override {
        QMainWindow::changeEvent(event);
        if (event->type() == QEvent::WindowStateChange && maximizeButton) {
            const bool maximized = isMaximized();
            maximizeButton->setText(maximized ? QString::fromUtf8("\xe2\x9d\x90") : QString::fromUtf8("\xe2\x96\xa1"));
            maximizeButton->setAccessibleName(maximized ? tr("Restore") : tr("Maximize"));
            maximizeButton->setToolTip(maximizeButton->accessibleName());
        }
    }

    bool eventFilter(QObject *watched, QEvent *event) override {
        auto *widget = qobject_cast<QWidget *>(watched);
        if (!widget || (widget != this && !isAncestorOf(widget))) return false;
        if (event->type() != QEvent::MouseButtonPress && event->type() != QEvent::MouseButtonDblClick)
            return false;
        auto *mouse = static_cast<QMouseEvent *>(event);
        const QPoint pos = mapFromGlobal(mouse->globalPosition().toPoint());
        if (mouse->button() == Qt::LeftButton && !isMaximized() && event->type() == QEvent::MouseButtonPress) {
            constexpr int border = 6;
            Qt::Edges edges;
            if (pos.x() < border) edges |= Qt::LeftEdge;
            if (pos.x() >= width() - border) edges |= Qt::RightEdge;
            if (pos.y() < border) edges |= Qt::TopEdge;
            if (pos.y() >= height() - border) edges |= Qt::BottomEdge;
            if (edges && windowHandle() && windowHandle()->startSystemResize(edges)) return true;
        }
        if (widget != titleBar) return false;
        if (mouse->button() == Qt::RightButton && event->type() == QEvent::MouseButtonPress) {
            showWindowMenu(mouse->globalPosition().toPoint());
            return true;
        }
        if (mouse->button() == Qt::LeftButton) {
            if (event->type() == QEvent::MouseButtonDblClick) toggleMaximized();
            else if (windowHandle()) windowHandle()->startSystemMove();
            return true;
        }
        return false;
    }

private:
    void toggleMaximized() { isMaximized() ? showNormal() : showMaximized(); }
    void showWindowMenu(const QPoint &position) {
        QMenu menu(this);
        auto *restore = menu.addAction(tr("Restore"), this, &QWidget::showNormal);
        restore->setEnabled(isMaximized() || isMinimized());
        auto *move = menu.addAction(tr("Move"), this, [this] { if (windowHandle()) windowHandle()->startSystemMove(); });
        move->setEnabled(!isMaximized());
        menu.addAction(tr("Minimize"), this, &QWidget::showMinimized);
        auto *maximize = menu.addAction(tr("Maximize"), this, &QWidget::showMaximized);
        maximize->setEnabled(!isMaximized());
        menu.addSeparator();
        menu.addAction(tr("Close"), this, &QWidget::close);
        menu.exec(position);
    }
    QWidget *titleBar = nullptr;
    QPushButton *maximizeButton = nullptr;
};

#endif