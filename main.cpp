#include "ImgWindow.h"

int main(int argc, char *argv[]) {

  QApplication app(argc, argv);
  ImgWindow *widget = new ImgWindow();

  widget->show();
  return app.exec();
}
