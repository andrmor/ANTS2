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

// WATER'S CHANGES
bool confirm(const QString & text, QWidget * parent)
{
    QMessageBox::StandardButton reply = QMessageBox::question(parent, "Confirmation request", text,
                                    QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
    if (reply == QMessageBox::Yes) return true;
    return false;
}

#else
void message(QString text)
{
    std::cout << text.toStdString() << std::endl;
}
#endif

/* // WWATER'S CHANGES
bool confirm(const QString & text, QWidget * parent)
{
    QMessageBox::StandardButton reply = QMessageBox::question(parent, "Confirmation request", text,
                                    QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
    if (reply == QMessageBox::Yes) return true;
    return false;
}
*/
