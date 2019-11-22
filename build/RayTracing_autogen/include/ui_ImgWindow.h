/********************************************************************************
** Form generated from reading UI file 'ImgWindow.ui'
**
** Created by: Qt User Interface Compiler version 5.13.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_IMGWINDOW_H
#define UI_IMGWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>

QT_BEGIN_NAMESPACE

class Ui_ImgWindow
{
public:
    QLabel *imgView;

    void setupUi(QMainWindow *ImgWindow)
    {
        if (ImgWindow->objectName().isEmpty())
            ImgWindow->setObjectName(QString::fromUtf8("ImgWindow"));
        ImgWindow->resize(480, 500);
        imgView = new QLabel(ImgWindow);
        imgView->setObjectName(QString::fromUtf8("imgView"));
        ImgWindow->setCentralWidget(imgView);

        retranslateUi(ImgWindow);

        QMetaObject::connectSlotsByName(ImgWindow);
    } // setupUi

    void retranslateUi(QMainWindow *ImgWindow)
    {
        ImgWindow->setWindowTitle(QCoreApplication::translate("ImgWindow", "ImgWindow", nullptr));
        imgView->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class ImgWindow: public Ui_ImgWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_IMGWINDOW_H
