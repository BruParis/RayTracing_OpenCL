#include "ImgWindow.h"
#include <QResizeEvent>
#include <QThread>

ImgWindow::ImgWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::ImgWindow) {

  std::cout << "MAIN WINDOW STARTED\n";

  ui->setupUi(this);
  this->setStyleSheet("background-color: black;");
  ui->imgView->setAlignment(Qt::AlignCenter);

  QThread *thread = new QThread;
  _worker = new ImgWindowWorker();
  _worker->moveToThread(thread);

  startTimer(1000 / 30);

  connect(thread, SIGNAL(started()), _worker, SLOT(startProcess()));
  connect(_worker, SIGNAL(newImgSignal()), this, SLOT(newImgSlot()));
  connect(_worker, SIGNAL(finishedSignal()), this, SLOT(finishedSlot()));

  thread->start();
}

ImgWindow::~ImgWindow() {
  delete _worker;
  delete ui;
}

void ImgWindow::resizeEvent(QResizeEvent *e) {
  auto size = e->size();
  QPoint pos(250, 50);
  // ui->imgView->move(pos);
  // ui->imgView->resize(this->width() - pos.x(), this->height() - pos.y());
}

void ImgWindow::timerEvent(QTimerEvent *e) {
  std::cout << "timer event\n";
  QMainWindow::timerEvent(e);
  if (!_finished) {
    std::cout << "  ---> fetch new image\n";
    _worker->copyImage(_image);
    ui->imgView->setPixmap(_image);
    _newImg = false;
  }
}

void ImgWindow::newImgSlot() {
  std::cout << "NEW IMAGE SLOT\n";
  _newImg = true;
}

void ImgWindow::finishedSlot() {
  std::cout << "FINISHED SLOT\n";
  _finished = true;
  QCoreApplication::quit();
}