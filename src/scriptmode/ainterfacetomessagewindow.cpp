#include "ainterfacetomessagewindow.h"
#include "ascriptmanager.h"
#include "ascriptmessengerdialog.h"

#include <QDebug>

AInterfaceToMessageWindow::AInterfaceToMessageWindow(AScriptManager* ScriptManager, QWidget* parent) :
    ScriptManager(ScriptManager), Parent(parent)
{
    DialogWidget = new AScriptMessengerDialog(parent);
    connectSignalSlots();

    Description = "Output to script message window.\n"
                  "If used inside multithread evaluation, every thread has its own window";
}

AInterfaceToMessageWindow::AInterfaceToMessageWindow(const AInterfaceToMessageWindow &other) :
   ScriptManager(other.ScriptManager), Parent(other.Parent)  //not a bug - pointer of MasterScriptManager in the clone!
{
    bGUIthread = false;
    DialogWidget = new AScriptMessengerDialog(other.Parent);
    connectSignalSlots();
}

void AInterfaceToMessageWindow::connectSignalSlots()
{
    Qt::ConnectionType ct = ( bGUIthread ? Qt::DirectConnection : Qt::QueuedConnection );

    connect(this, &AInterfaceToMessageWindow::requestAppend, DialogWidget, &AScriptMessengerDialog::Append, ct);
    connect(this, &AInterfaceToMessageWindow::requestClear, DialogWidget, &AScriptMessengerDialog::Clear, ct);
    connect(this, &AInterfaceToMessageWindow::requestShow, DialogWidget, &AScriptMessengerDialog::Show, ct);
    connect(this, &AInterfaceToMessageWindow::requestHide, DialogWidget, &AScriptMessengerDialog::Hide, ct);
    connect(this, &AInterfaceToMessageWindow::requestTemporaryShow, DialogWidget, &AScriptMessengerDialog::ShowTemporary, ct);
    connect(this, &AInterfaceToMessageWindow::requestSetTransparency, DialogWidget, &AScriptMessengerDialog::SetTransparent, ct);
    connect(this, &AInterfaceToMessageWindow::requestSetTitle, DialogWidget, &AScriptMessengerDialog::SetDialogTitle, ct);
    connect(this, &AInterfaceToMessageWindow::requestMove, DialogWidget, &AScriptMessengerDialog::Move, ct);
    connect(this, &AInterfaceToMessageWindow::requestResize, DialogWidget, &AScriptMessengerDialog::Resize, ct);
    connect(this, &AInterfaceToMessageWindow::requestSetFontSize, DialogWidget, &AScriptMessengerDialog::SetFontSize, ct);
}

AInterfaceToMessageWindow::~AInterfaceToMessageWindow()
{
  //    qDebug() << "Msg destructor. Master?"<<bGUIthread;

  if (bGUIthread)
  {
      //    qDebug() << "Delete message dialog for master triggered!";
      delete DialogWidget; DialogWidget = 0;
  }
  //Clones - DO NOT delete the dialogs -> they are handled by the ScriptManager of the GUI thread
}

void AInterfaceToMessageWindow::ReplaceDialogWidget(AScriptMessengerDialog *AnotherDialogWidget)
{
    delete DialogWidget;
    DialogWidget = AnotherDialogWidget;
    connectSignalSlots();
}

AScriptMessengerDialog *AInterfaceToMessageWindow::GetDialogWidget()
{
    return DialogWidget;
}

void AInterfaceToMessageWindow::Append(const QString &text)
{
    emit requestAppend(text);
}

void AInterfaceToMessageWindow::Clear()
{
    emit requestClear();
}

void AInterfaceToMessageWindow::Show()
{
    emit requestShow();
}

void AInterfaceToMessageWindow::Hide()
{
    emit requestHide();
}

void AInterfaceToMessageWindow::Show(const QString &text, int ms)
{
    emit requestAppend(text);
    emit requestTemporaryShow(ms);
}

void AInterfaceToMessageWindow::SetTransparent(bool flag)
{
    emit requestSetTransparency(flag);
}

void AInterfaceToMessageWindow::SetDialogTitle(const QString &title)
{
    emit requestSetTitle(title);
}

void AInterfaceToMessageWindow::Move(double x, double y)
{
    emit requestMove(x, y);
}

void AInterfaceToMessageWindow::Resize(double w, double h)
{
    emit requestResize(w, h);
}

void AInterfaceToMessageWindow::SetFontSize(int size)
{
    emit requestSetFontSize(size);
}

void AInterfaceToMessageWindow::ShowAllThreadMessengers()
{
    if (bGUIthread) ScriptManager->showAllMessengerWidgets();
}

void AInterfaceToMessageWindow::HideAllThreadMessengers()
{
    if (bGUIthread) ScriptManager->hideAllMessengerWidgets();
}

void AInterfaceToMessageWindow::RestorelWidget()
{
    if (bGUIthread)
    {
        if (DialogWidget->IsShown())
            DialogWidget->RestoreWidget();
    }        
}

void AInterfaceToMessageWindow::HideWidget()
{
    if (bGUIthread)
    {
        if (DialogWidget->IsShown())
            DialogWidget->Hide();
    }
}
