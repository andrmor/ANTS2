#include "ainterfacetomessagewindow.h"
#include "ascriptmanager.h"

#include <QWidget>
#include <QTime>
#include <QApplication>
#include <QDialog>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QDebug>

AInterfaceToMessageWindow::AInterfaceToMessageWindow(AScriptManager* ScriptManager, QWidget* parent) :
   ScriptManager(ScriptManager), D(0), Parent(parent)
{
  bEnabled = true;
  bActivated = false;

  init(false);
}

AInterfaceToMessageWindow::AInterfaceToMessageWindow(const AInterfaceToMessageWindow &other) :
   ScriptManager(other.ScriptManager), D(0), Parent(other.Parent)  //not a bug - pointer of MasterScriptManager in the clone!
{
    bMasterCopy = false;

    bEnabled = true;
    bActivated = false;

    init(false);
}

void AInterfaceToMessageWindow::init(bool fTransparent)
{
  D = new QDialog(Parent);
  if (bMasterCopy) QObject::connect(D, &QDialog::finished, this, &AInterfaceToMessageWindow::Hide);

  QVBoxLayout* l = new QVBoxLayout;
  e = new QPlainTextEdit();
  e->setReadOnly(true);
  l->addWidget(e);
  D->setLayout(l);

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
  qDebug() << "Msg destructor. Master?"<<bMasterCopy;

  if (bMasterCopy) deleteDialog();
}

void AInterfaceToMessageWindow::SetTransparent(bool flag)
{
  if (!bMasterCopy) return; //cannot call again init()

  QString text = e->document()->toPlainText();
  delete D;
  D = 0;
  init(flag);
  e->setPlainText(text);
}

void AInterfaceToMessageWindow::SetDialogTitle(const QString &title)
{
    D->setWindowTitle(title);
}

void AInterfaceToMessageWindow::Append(QString txt)
{
    if (bMasterCopy)
        e->appendHtml(txt);
    else
        emit requestAppendMsg(this, txt);
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
  X = x; Y = y;
  D->move(X, Y);
}

void AInterfaceToMessageWindow::Resize(double w, double h)
{
  WW = w; HH = h;
  D->resize(WW, HH);
}

void AInterfaceToMessageWindow::Show()
{
  if (!bEnabled) return;

  if (bMasterCopy)
  {
      D->show();
      D->raise();
  }
  else
  {
      emit requestShowDialog(D);
  }

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
   qDebug() << "Delete message dialog triggered!" ;
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


AMessengerDialog::AMessengerDialog(QWidget *parent)
{

}
