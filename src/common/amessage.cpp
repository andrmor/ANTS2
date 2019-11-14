#include "amessage.h"

#ifdef GUI
#include <QMessageBox>
#include <QInputDialog>

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

bool confirm(const QString & text, QWidget * parent)
{
    QMessageBox::StandardButton reply = QMessageBox::question(parent, "Confirmation request", text,
                                    QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
    if (reply == QMessageBox::Yes) return true;
    return false;
}

void inputInteger(const QString &text, int &input, int min, int max, QWidget *parent)
{
    bool ok;
    int res = QInputDialog::getInt(parent, "", text, input, min, max, 1, &ok);
    if (ok) input = res;
}
