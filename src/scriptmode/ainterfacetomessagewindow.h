#ifndef AINTERFACETOMESSAGEWINDOW_H
#define AINTERFACETOMESSAGEWINDOW_H

#include "ascriptinterface.h"

#include <QVector>
#include <QWidget>

class AScriptManager;
class QDialog;
class QPlainTextEdit;
//class QWidget;

class AMessengerDialog : QWidget
{
    Q_OBJECT

public:
    AMessengerDialog(QWidget *parent);

    QPlainTextEdit* e;
};


class AInterfaceToMessageWindow : public AScriptInterface
{
  Q_OBJECT

public:
  AInterfaceToMessageWindow(AScriptManager *ScriptManager, QWidget *parent);
  AInterfaceToMessageWindow(const AInterfaceToMessageWindow& other);
  ~AInterfaceToMessageWindow();

  bool IsMultithreadCapable() const override {return true;}

  QDialog *D;
  double X = 50, Y = 50;
  double WW = 300, HH = 500;

  QPlainTextEdit* e;

  bool bEnabled;

public slots:
  void Enable(bool flag) {bEnabled = flag;}
  void Append(QString txt);
  void Clear();
  void Show();
  void Hide();
  void Show(QString txt, int ms = -1);
  void SetTransparent(bool flag);
  void SetDialogTitle(const QString& title);

  void Move(double x, double y);
  void Resize(double w, double h);

  void SetFontSize(int size);

public:
  void deleteDialog();
  bool isActive() {return bActivated;}
  void hide();     //does not affect bActivated status
  void restore();  //does not affect bActivated status

private:
  AScriptManager* ScriptManager;
  QWidget* Parent;
  bool bActivated;

  bool bMasterCopy = true;
  QVector<QDialog*> ThreadDialogs;

  void init(bool fTransparent);

signals:
  void requestShowDialog(QDialog* dialog);
  void requestAppendMsg(AInterfaceToMessageWindow* msg, const QString& text);
};

#endif // AINTERFACETOMESSAGEWINDOW_H
