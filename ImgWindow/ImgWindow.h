#ifndef IMG_WINDOW_H
#define IMG_WINDOW_H

#include <QMainWindow>
#include <iostream>

#include "ImgWindowWorker.h"
#include "ui_ImgWindow.h"

class ImgWindow : public QMainWindow {
  Q_OBJECT

public:
  ImgWindow(QWidget *parent = nullptr);
  ~ImgWindow();

private:
  bool _newImg = false;
  QPixmap _image;
  Ui::ImgWindow *ui;
  ImgWindowWorker *_worker;

  void resizeEvent(QResizeEvent *e) override;
  void timerEvent(QTimerEvent *e) override;

private slots:
  void newImgSlot();
};

#endif