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

private:
  void saveImage(int imgIdx);
  CLOperator* _clOperator;
  const Scene* _scene;
  Camera* _cam;

private slots:
  void startProcess();

signals:
  void finished();
  void error(QString err);
};
#endif