#include "ImgWindow.h"
#include <QThread>

ImgWindow::ImgWindow(QWidget *parent) : QWidget(parent) {

  std::cout << "MAIN WINDOW STARTED\n";

  // ui->setupUi(this);

  QThread *thread = new QThread;
  _worker = new ImgWindowWorker();
  _worker->moveToThread(thread);

  connect(thread, SIGNAL (started()), _worker, SLOT (startProcess()));

  thread->start();
}

ImgWindow::~ImgWindow() {
  delete _worker;
  delete ui;
}