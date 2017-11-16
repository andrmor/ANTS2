#include "ainterfacetomessagewindow.h"

#include <QWidget>
#include <QTime>
#include <QApplication>
#include <QDialog>
#include <QPlainTextEdit>
#include <QVBoxLayout>

static int msgH = 500, msgW = 300, msgX=50, msgY=50;

AInterfaceToMessageWindow::AInterfaceToMessageWindow(QWidget* parent) : D(0), Parent(parent)
{
  bEnabled = true;
  bActivated = false;
  init(false);
}

void AInterfaceToMessageWindow::init(bool fTransparent)
{
  D = new QDialog(Parent);
  QObject::connect(D, &QDialog::finished, this, &AInterfaceToMessageWindow::Hide);

  QVBoxLayout* l = new QVBoxLayout;
  e = new QPlainTextEdit();
  e->setReadOnly(true);
  l->addWidget(e);
  D->setLayout(l);

  X = msgX;
  Y = msgY;
  WW = msgW;
  HH = msgH;

  D->setGeometry(X, Y, WW, HH);
  D->setWindowTitle("Script msg");

  if (fTransparent)
    {
      D->setWindowFlags(Qt::FramelessWindowHint);
      D->setAttribute(Qt::WA_TranslucentBackground);

      e->setStyleSheet("background: rgba(0,0,255,0%)");
      e->setFrameStyle(QFrame::NoFrame);
    }
}


AInterfaceToMessageWindow::~AInterfaceToMessageWindow()
{
  //qDebug() << "Msg destructor";
  deleteDialog();
}

void AInterfaceToMessageWindow::SetTransparent(bool flag)
{
  QString text = e->document()->toPlainText();
  delete D;
  D = 0;
  init(flag);
  e->setPlainText(text);
}

void AInterfaceToMessageWindow::Append(QString txt)
{
  e->appendHtml(txt);
}

void AInterfaceToMessageWindow::Clear()
{
  e->clear();
}

void AInterfaceToMessageWindow::Show(QString txt, int ms)
{
  if (!bEnabled) return;
  e->clear();
  e->appendHtml(txt);

  if (ms == -1)
    {
      D->show();
      D->raise();
      bActivated = true;
      return;
    }

  D->show();
  D->raise();
  bActivated = true;
  QTime t;
  t.restart();
  do qApp->processEvents();
  while (t.elapsed()<ms);
  D->hide();
  bActivated = false;
}

void AInterfaceToMessageWindow::Move(double x, double y)
{
  X = msgX = x; Y = msgY = y;
  D->move(X, Y);
}

void AInterfaceToMessageWindow::Resize(double w, double h)
{
  WW = msgW = w; HH = msgH = h;
  D->resize(WW, HH);
}

void AInterfaceToMessageWindow::Show()
{
  if (!bEnabled) return;
  D->show();
  D->raise();
  bActivated = true;
}

void AInterfaceToMessageWindow::Hide()
{
  D->hide();
  bActivated = false;
}

void AInterfaceToMessageWindow::SetFontSize(int size)
{
  QFont f = e->font();
  f.setPointSize(size);
  e->setFont(f);
}

void AInterfaceToMessageWindow::deleteDialog()
{
   delete D;
   D = 0;
}

void AInterfaceToMessageWindow::hide()
{
    if (D) D->hide();
}

void AInterfaceToMessageWindow::restore()
{
    if (D) D->show();
}

