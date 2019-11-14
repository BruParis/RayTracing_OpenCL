#ifndef IMG_WINDOW_H
#define IMG_WINDOW_H

#include <QWidget>
#include <iostream>

#include "ImgWindowWorker.h"
#include "ui_ImgWindow.h"

class ImgWindow : public QWidget {
  Q_OBJECT

public:
  ImgWindow(QWidget *parent = nullptr);
  ~ImgWindow();

private:
  Ui::ImgWindow *ui;
  ImgWindowWorker *_worker;
};

#endif