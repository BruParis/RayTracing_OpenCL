#include "ImgWindow.h"
#include <QThread>
#include <QResizeEvent>

ImgWindow::ImgWindow(QWidget *parent) : QMainWindow(parent) {

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

void ImgWindow::resizeEvent(QResizeEvent * e)
{
	auto size = e->size();
	QPoint pos(250, 50);
	// ui->imgView->move(pos);
	// ui->imgView->resize(this->width() - pos.x(), this->height() - pos.y());
}


void ImgWindow::timerEvent(QTimerEvent * e)
{
  std::cout << "timer event\n"; 
	// if ( new image ) {
	//	render->signal.hasNew = false;
	//	render->copyImage(this->image);
	//	ui->imgView->setPixmap(image);
	//}
	
}