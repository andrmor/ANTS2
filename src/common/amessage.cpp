#include "amessage.h"

#ifdef GUI
#include <QMessageBox>

void message(QString text, QWidget* parent)
{
  QMessageBox mb(parent);
  mb.setWindowFlags(mb.windowFlags() | Qt::WindowStaysOnTopHint);
  mb.setText(text);
  if (!parent) mb.move(200,200);
  mb.exec();
}
#else
void message(QString text)
{
    std::cout << text.toStdString() << std::endl;
}
#endif



