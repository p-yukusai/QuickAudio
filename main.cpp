#include <QApplication>
#include <QDebug>
#include <QStatusBar>
#include <QSignalBlocker>


#include "src/imm_audio.h"
#include "src/frameless_window.h"
#include "src/main_window.h"
#include "src/volume_dial.h"
#include "src/volume_slider.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setFont(QFont("Segoe UI"));
    app.setWindowIcon(QIcon(":/res/icon.png"));
    FramelessWindow qmw;
    UI::MainWindow main_window;
    main_window.setupUi(&qmw);
    qmw.setWindowTitle(QObject::tr("QuickAudio"));
    qmw.resize(620, 620);
    qmw.setMinimumSize(480, 560);
    auto replaceDial = [&](QDial *&dial) {
        auto *replacement = new VolumeDial(main_window.centralwidget);
        replacement->setObjectName(dial->objectName());
        delete main_window.gridLayout_2->replaceWidget(dial, replacement);
        delete dial;
        dial = replacement;
    };
    replaceDial(main_window.leftDial);
    replaceDial(main_window.rightDial);
    auto replaceSlider = [&](QSlider *&slider) {
        auto *replacement = new VolumeSlider(main_window.centralwidget);
        replacement->setObjectName(slider->objectName());
        delete main_window.gridLayout_2->replaceWidget(slider, replacement);
        delete slider;
        slider = replacement;
    };
    replaceSlider(main_window.leftSlider);
    replaceSlider(main_window.rightSlider);
    main_window.gridLayout_2->setContentsMargins(42, 20, 42, 18);
    main_window.gridLayout_2->setHorizontalSpacing(48);
    main_window.gridLayout_2->setVerticalSpacing(12);
    main_window.gridLayout_2->setAlignment(main_window.leftDial, Qt::AlignCenter);
    main_window.gridLayout_2->setAlignment(main_window.rightDial, Qt::AlignCenter);
    main_window.gridLayout_2->setRowStretch(8, 1);
    main_window.label->setObjectName("channelLabel");
    main_window.label_2->setObjectName("channelLabel");
    main_window.leftSlider->setRange(0, 100);
    main_window.rightSlider->setRange(0, 100);
    main_window.leftDial->setRange(0, 100);
    main_window.rightDial->setRange(0, 100);
    QObject::connect(main_window.leftSlider, &QSlider::valueChanged, main_window.leftDial, &QDial::setValue);
    QObject::connect(main_window.leftDial, &QDial::valueChanged, main_window.leftSlider, &QSlider::setValue);
    QObject::connect(main_window.rightSlider, &QSlider::valueChanged, main_window.rightDial, &QDial::setValue);
    QObject::connect(main_window.rightDial, &QDial::valueChanged, main_window.rightSlider, &QSlider::setValue);

    main_window.gridLayout_2->removeItem(main_window.verticalSpacer_3);
    main_window.gridLayout_2->addItem(main_window.verticalSpacer_3, 8, 0);
    auto *leftVolumeLabel = new QLabel(main_window.centralwidget);
    auto *rightVolumeLabel = new QLabel(main_window.centralwidget);
    auto *masterVolumeLabel = new QLabel(main_window.centralwidget);
    auto *masterSlider = new VolumeSlider(main_window.centralwidget);
    masterSlider->setObjectName("masterSlider");
    masterSlider->setAccessibleName(QObject::tr("System volume"));
    masterSlider->setRange(0, 100);
    auto *deviceLabel = new QLabel(main_window.centralwidget);
    leftVolumeLabel->setObjectName("volumeValue");
    rightVolumeLabel->setObjectName("volumeValue");
    masterVolumeLabel->setObjectName("masterVolume");
    deviceLabel->setObjectName("deviceLabel");
    deviceLabel->setWordWrap(true);
    for (auto *label : {leftVolumeLabel, rightVolumeLabel, masterVolumeLabel, deviceLabel})
        label->setAlignment(Qt::AlignCenter);
    main_window.gridLayout_2->addWidget(leftVolumeLabel, 4, 0);
    main_window.gridLayout_2->addWidget(rightVolumeLabel, 4, 1);
    main_window.gridLayout_2->addWidget(masterVolumeLabel, 5, 0, 1, 2);
    main_window.gridLayout_2->addWidget(masterSlider, 6, 0, 1, 2);
    main_window.gridLayout_2->addWidget(deviceLabel, 7, 0, 1, 2);

    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    struct ComCleanup {
        HRESULT result;
        ~ComCleanup() { if (SUCCEEDED(result)) CoUninitialize(); }
    } comCleanup{comResult};
    ImmAudio audio;
    const auto refresh = [&]() {
        std::vector<float> volumes;
        const HRESULT result = audio.getVolumes(volumes);
        const bool available = SUCCEEDED(result) && !volumes.empty();
        const bool hasRightChannel = available && volumes.size() > 1;
        main_window.leftSlider->setEnabled(available);
        main_window.leftDial->setEnabled(available);
        main_window.rightSlider->setEnabled(hasRightChannel);
        main_window.rightDial->setEnabled(hasRightChannel);
        const QSignalBlocker leftSliderBlock(main_window.leftSlider);
        const QSignalBlocker leftDialBlock(main_window.leftDial);
        const QSignalBlocker rightSliderBlock(main_window.rightSlider);
        const QSignalBlocker rightDialBlock(main_window.rightDial);
        const int left = available ? qRound(volumes[0] * 100) : 0;
        const int right = hasRightChannel ? qRound(volumes[1] * 100) : 0;
        main_window.leftSlider->setValue(left);
        main_window.leftDial->setValue(left);
        main_window.rightSlider->setValue(right);
        main_window.rightDial->setValue(right);
        leftVolumeLabel->setText(available ? QObject::tr("%1%").arg(left) : QObject::tr("Unavailable"));
        rightVolumeLabel->setText(hasRightChannel ? QObject::tr("%1%").arg(right) : QObject::tr("Unavailable"));
        float master = 0;
        const bool masterAvailable = SUCCEEDED(audio.masterVolume(master));
        const QSignalBlocker masterSliderBlock(masterSlider);
        masterSlider->setEnabled(masterAvailable);
        masterSlider->setValue(masterAvailable ? qRound(master * 100) : 0);
        masterVolumeLabel->setText(masterAvailable
                                       ? QObject::tr("System volume: %1%").arg(qRound(master * 100))
                                       : QObject::tr("System volume: unavailable"));
        const QString name = audio.deviceName();
        deviceLabel->setText(name.isEmpty() ? QObject::tr("Output device: unavailable")
                                            : QObject::tr("Output device: %1").arg(name));
        qmw.statusBar()->showMessage(available ? QObject::tr("Controlling system output")
                                               : QObject::tr("No output device available"));
    };
    QObject::connect(&audio, &ImmAudio::stateChanged, &qmw, refresh);
    const auto setChannel = [&audio, &qmw, &refresh](UINT32 channel, int value) {
        const HRESULT result = audio.setVolume(channel, value / 100.0f);
        if (FAILED(result)) {
            refresh();
            qmw.statusBar()->showMessage(QObject::tr("Could not set channel volume (0x%1)")
                                             .arg(static_cast<unsigned long>(result), 8, 16, QLatin1Char('0')));
        }
    };
    QObject::connect(main_window.leftSlider, &QSlider::valueChanged, &qmw,
                     [setChannel](int value) { setChannel(0, value); });
    QObject::connect(main_window.rightSlider, &QSlider::valueChanged, &qmw,
                     [setChannel](int value) { setChannel(1, value); });
    QObject::connect(masterSlider, &QSlider::valueChanged, &qmw, [&](int value) {
        const HRESULT result = audio.setMasterVolume(value / 100.0f);
        if (FAILED(result)) {
            refresh();
            qmw.statusBar()->showMessage(QObject::tr("Could not set system volume (0x%1)")
                                             .arg(static_cast<unsigned long>(result), 8, 16, QLatin1Char('0')));
        } else {
            masterVolumeLabel->setText(QObject::tr("System volume: %1%").arg(value));
        }
    });
    const HRESULT audioResult = SUCCEEDED(comResult) ? audio.initialize() : comResult;
    refresh();
    if (FAILED(audioResult)) {
        qmw.statusBar()->showMessage(QObject::tr("Audio initialization failed (0x%1)")
                                         .arg(static_cast<unsigned long>(audioResult), 8, 16, QLatin1Char('0')));
        qWarning() << "Audio initialization failed:" << Qt::hex << audioResult;
    }

    app.setStyle("Fusion");
    app.setStyleSheet(R"(
        QMainWindow { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #40314d, stop:0.22 #353451, stop:0.45 #273b50, stop:0.68 #34344e, stop:0.84 #422f49, stop:1 #252d45); color: #e8e7f5; }
        QWidget#centralwidget, QWidget#titleBar { background: transparent; }
        QLabel#windowTitle { font-size: 13px; font-weight: 600; }
        QWidget#titleBar QPushButton { background: transparent; color: #cbd5e1; border: 0; font-size: 18px; }
        QWidget#titleBar QPushButton:hover { background: #39345a; }
        QWidget#titleBar QPushButton#closeButton:hover { background: #c42b1c; color: white; }
        QLabel { color: #e2e8f0; font-family: "Segoe UI"; font-size: 15px; }
        QLabel#channelLabel { color: #a7a5c5; font-size: 11px; font-weight: 600; }
        QLabel#volumeValue { color: #b6a4ff; font-size: 15px; font-weight: 600; }
        QLabel#masterVolume { color: #ebe9ff; font-size: 16px; font-weight: 600;
                              background: #25213f; border: 1px solid #393455; border-radius: 10px; padding: 10px; }
        QLabel#deviceLabel { color: #a7a5c5; font-size: 12px; }
        QStatusBar { background: #0d1022; color: #a7a5c5; }
        QMenu { background: #211e38; color: #e8e7f5; border: 1px solid #454064; }
        QMenu::item:selected { background: #45406d; }
    )");
    qmw.show();
    return QApplication::exec();
}
