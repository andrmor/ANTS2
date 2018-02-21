#ifndef AINTERFACETOMESSAGEWINDOW_H
#define AINTERFACETOMESSAGEWINDOW_H

#include "ascriptinterface.h"

class QDialog;
class QPlainTextEdit;
class QWidget;

class AInterfaceToMessageWindow : public AScriptInterface
{
  Q_OBJECT

public:
  AInterfaceToMessageWindow(QWidget *parent);
  ~AInterfaceToMessageWindow();

  QDialog *D;
  double X, Y;
  double WW, HH;

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

  void Move(double x, double y);
  void Resize(double w, double h);

  void SetFontSize(int size);

public:
  void deleteDialog();
  bool isActive() {return bActivated;}
  void hide();     //does not affect bActivated status
  void restore();  //does not affect bActivated status

private:
  QWidget* Parent;
  bool bActivated;

  void init(bool fTransparent);
};

#endif // AINTERFACETOMESSAGEWINDOW_H
