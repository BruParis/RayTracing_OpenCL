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
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ImgWindow
{
public:
    QLabel *label;

    void setupUi(QWidget *Dialog)
    {
        if (Dialog->objectName().isEmpty())
            Dialog->setObjectName(QString::fromUtf8("Dialog"));
        Dialog->resize(400, 300);
        label = new QLabel(Dialog);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(10, 14, 381, 271));

        retranslateUi(Dialog);

        QMetaObject::connectSlotsByName(Dialog);
    } // setupUi

    void retranslateUi(QWidget *Dialog)
    {
        Dialog->setWindowTitle(QCoreApplication::translate("ImgWindow", "ImgWindow", nullptr));
        label->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class ImgWindow: public Ui_ImgWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_IMGWINDOW_H
