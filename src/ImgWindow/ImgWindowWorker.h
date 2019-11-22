#ifndef IMG_WINDOW_WORKER_H
#define IMG_WINDOW_WORKER_H

#include <QWidget>
#include <fstream>
#include <iostream>

#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/cl.hpp>
#include "CLOperator/CLOperator.h"

#define ANTI_ALIASING_SAMPLES 1
#define PI 3.14159

class ImgWindowWorker : public QObject {
  Q_OBJECT
public:
  ImgWindowWorker();
  ~ImgWindowWorker();

  void copyImage(QPixmap &pixmap);

private:
  int _imgIdx = 0;
  CLOperator* _clOperator;
  const Scene* _scene;
  Camera* _cam;
  unsigned char * _pixelBuffer;

private slots:
  void startProcess();

signals:
  void finished();
  void newImgSignal();
  void error(QString err);
};
#endif