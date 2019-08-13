#ifndef AINTERFACETOMESSAGEWINDOW_H
#define AINTERFACETOMESSAGEWINDOW_H

#include "ascriptinterface.h"

#include <QObject>
#include <QVector>
#include <QString>

class AScriptManager;
class AScriptMessengerDialog;

class AMsg_SI : public AScriptInterface
{
  Q_OBJECT

public:
  AMsg_SI(AScriptManager *ScriptManager, QWidget *parent);
  AMsg_SI(const AMsg_SI& other);
  ~AMsg_SI();

  bool IsMultithreadCapable() const override {return true;}

  // for handling with the Scriptmanager of the GUI thread
  void ReplaceDialogWidget(AScriptMessengerDialog* AnotherDialogWidget);
  AScriptMessengerDialog* GetDialogWidget();

public slots:
  void Append(const QString& text);
  void Clear();
  void Show();
  void Hide();
  void Show(const QString& text, int ms = -1);
  void SetDialogTitle(const QString& title);

  void Move(double x, double y);
  void Resize(double w, double h);

  void SetFontSize(int size);

  void ShowAllThreadMessengers();
  void HideAllThreadMessengers();

signals:
  void requestAppend(const QString& text);
  void requestClear();
  void requestShow();
  void requestHide();
  void requestTemporaryShow(int ms);
  void requestSetTitle(const QString& title);
  void requestMove(double x, double y);
  void requestResize(double w, double h);
  void requestSetFontSize(int size);

public:
  void RestorelWidget();  //on script window restore
  void HideWidget();     //on script window hide

private:
  AScriptManager* ScriptManager;
  QWidget* Parent;
  AScriptMessengerDialog* DialogWidget;

  void connectSignalSlots();

};

#endif // AINTERFACETOMESSAGEWINDOW_H
