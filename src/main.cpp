#include "ImgWindow/ImgWindow.h"

int main(int argc, char *argv[]) {

  QApplication app(argc, argv);
  ImgWindow *widget = new ImgWindow();

  widget->show();
  int ret = app.exec();

  delete widget;

  return ret;
}
