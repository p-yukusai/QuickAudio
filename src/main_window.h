/********************************************************************************
** Form generated from reading UI file 'designerUfbyzd.ui'
**
** Created by: Qt User Interface Compiler version 6.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef DESIGNERUFBYZD_H
#define DESIGNERUFBYZD_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDial>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class mainWindow
{
public:
    QWidget *centralwidget;
    QGridLayout *gridLayout_2;
    QSlider *leftSlider;
    QLabel *label_2;
    QDial *rightDial;
    QDial *leftDial;
    QLabel *label;
    QSpacerItem *verticalSpacer_4;
    QSpacerItem *verticalSpacer_3;
    QSlider *rightSlider;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        gridLayout_2 = new QGridLayout(centralwidget);
        gridLayout_2->setObjectName("gridLayout_2");
        leftSlider = new QSlider(centralwidget);
        leftSlider->setObjectName("leftSlider");
        leftSlider->setOrientation(Qt::Orientation::Horizontal);

        gridLayout_2->addWidget(leftSlider, 2, 0, 1, 1);

        label_2 = new QLabel(centralwidget);
        label_2->setObjectName("label_2");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
        label_2->setSizePolicy(sizePolicy);
        label_2->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayout_2->addWidget(label_2, 3, 1, 1, 1);

        rightDial = new QDial(centralwidget);
        rightDial->setObjectName("rightDial");

        gridLayout_2->addWidget(rightDial, 1, 1, 1, 1);

        leftDial = new QDial(centralwidget);
        leftDial->setObjectName("leftDial");

        gridLayout_2->addWidget(leftDial, 1, 0, 1, 1);

        label = new QLabel(centralwidget);
        label->setObjectName("label");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Maximum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy1);
        label->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayout_2->addWidget(label, 3, 0, 1, 1);

        auto *volumeLabel = new QLabel(QObject::tr("Volume"), centralwidget);
        volumeLabel->setAlignment(Qt::AlignCenter);
        volumeLabel->setSizePolicy(sizePolicy1);
        gridLayout_2->addWidget(volumeLabel, 0, 0, 1, 2);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);

        gridLayout_2->addItem(verticalSpacer_3, 4, 0, 1, 1);

        rightSlider = new QSlider(centralwidget);
        rightSlider->setObjectName("rightSlider");
        rightSlider->setOrientation(Qt::Orientation::Horizontal);

        gridLayout_2->addWidget(rightSlider, 2, 1, 1, 1);

        MainWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "RIGHT CHANNEL", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "LEFT CHANNEL", nullptr));
    } // retranslateUi

};

namespace UI {
    class MainWindow: public mainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // DESIGNERUFBYZD_H
