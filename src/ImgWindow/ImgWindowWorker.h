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
  ImgWindowWorker(int imageW, int imageH);
  ~ImgWindowWorker();

  void copyImage(QPixmap &pixmap);

private:
  int _imageW;
  int _imageH;
  int _imgIdx = 0;
  cl_float4 * _cpu_output;
  unsigned char * _pixelBuffer;

  Camera* _cam;
  const Scene* _scene;
  CLOperator* _clOperator;

private slots:
  void startProcess();

signals:
  void finishedSignal();
  void newImgSignal();
  void error(QString err);
};
#endif