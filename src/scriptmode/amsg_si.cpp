#include "amsg_si.h"
#include "ajavascriptmanager.h"
#include "ascriptmessengerdialog.h"

#include <QDebug>

AMsg_SI::AMsg_SI(AScriptManager *ScriptManager, QWidget* parent) :
    ScriptManager(ScriptManager), Parent(parent)
{
    DialogWidget = new AScriptMessengerDialog(parent);
    connectSignalSlots();

    Description = "Output to script message window.\n"
                  "If used inside multithread evaluation, every thread has its own window";
}

AMsg_SI::AMsg_SI(const AMsg_SI &other) :
   AScriptInterface(other),
   ScriptManager(other.ScriptManager), Parent(other.Parent)  //not a bug - pointer of MasterScriptManager in the clone!
{
    DialogWidget = new AScriptMessengerDialog(other.Parent);
    connectSignalSlots();
}

void AMsg_SI::connectSignalSlots()
{
    Qt::ConnectionType ct = ( bGuiThread ? Qt::DirectConnection : Qt::QueuedConnection );

    connect(this, &AMsg_SI::requestAppend, DialogWidget, &AScriptMessengerDialog::Append, ct);
    connect(this, &AMsg_SI::requestClear, DialogWidget, &AScriptMessengerDialog::Clear, ct);
    connect(this, &AMsg_SI::requestShow, DialogWidget, &AScriptMessengerDialog::Show, ct);
    connect(this, &AMsg_SI::requestHide, DialogWidget, &AScriptMessengerDialog::Hide, ct);
    connect(this, &AMsg_SI::requestTemporaryShow, DialogWidget, &AScriptMessengerDialog::ShowTemporary, ct);
    connect(this, &AMsg_SI::requestSetTitle, DialogWidget, &AScriptMessengerDialog::SetDialogTitle, ct);
    connect(this, &AMsg_SI::requestMove, DialogWidget, &AScriptMessengerDialog::Move, ct);
    connect(this, &AMsg_SI::requestResize, DialogWidget, &AScriptMessengerDialog::Resize, ct);
    connect(this, &AMsg_SI::requestSetFontSize, DialogWidget, &AScriptMessengerDialog::SetFontSize, ct);
}

AMsg_SI::~AMsg_SI()
{
  //    qDebug() << "Msg destructor. Master?"<<bGUIthread;

  if (bGuiThread)
  {
      //    qDebug() << "Delete message dialog for master triggered!";
      delete DialogWidget; DialogWidget = 0;
  }
  //Clones - DO NOT delete the dialogs -> they are handled by the ScriptManager of the GUI thread
}

void AMsg_SI::ReplaceDialogWidget(AScriptMessengerDialog *AnotherDialogWidget)
{
    delete DialogWidget;
    DialogWidget = AnotherDialogWidget;
    connectSignalSlots();
}

AScriptMessengerDialog *AMsg_SI::GetDialogWidget()
{
    return DialogWidget;
}

void AMsg_SI::Append(const QString &text)
{
    emit requestAppend(text);
}

void AMsg_SI::Clear()
{
    emit requestClear();
}

void AMsg_SI::Show()
{
    emit requestShow();
}

void AMsg_SI::Hide()
{
    emit requestHide();
}

void AMsg_SI::Show(const QString &text, int ms)
{
    emit requestClear();
    emit requestAppend(text);
    emit requestTemporaryShow(ms);
}

void AMsg_SI::SetDialogTitle(const QString &title)
{
    emit requestSetTitle(title);
}

void AMsg_SI::Move(double x, double y)
{
    emit requestMove(x, y);
}

void AMsg_SI::Resize(double w, double h)
{
    emit requestResize(w, h);
}

void AMsg_SI::SetFontSize(int size)
{
    emit requestSetFontSize(size);
}

void AMsg_SI::ShowAllThreadMessengers()
{
    if (bGuiThread)
      {
        AJavaScriptManager* JSM = dynamic_cast<AJavaScriptManager*>(ScriptManager);
        if (JSM) JSM->showAllMessengerWidgets();
      }
}

void AMsg_SI::HideAllThreadMessengers()
{
    if (bGuiThread)
      {
        AJavaScriptManager* JSM = dynamic_cast<AJavaScriptManager*>(ScriptManager);
        if (JSM) JSM->hideAllMessengerWidgets();
      }
}

void AMsg_SI::RestorelWidget()
{
    if (bGuiThread)
    {
        if (DialogWidget->IsShown())
            DialogWidget->RestoreWidget();
    }        
}

void AMsg_SI::HideWidget()
{
    if (bGuiThread)
    {
        if (DialogWidget->IsShown())
            DialogWidget->Hide();
    }
}
