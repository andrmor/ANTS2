#include "credits.h"
#include "ui_credits.h"

#include <QtGui>

Credits::Credits(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::Credits)
{
  ui->setupUi(this);
  setWindowTitle("Credits");
}

Credits::~Credits()
{
  delete ui;
}

void Credits::on_label_10_linkActivated(const QString &link)
{
    QDesktopServices::openUrl ( link );
}
