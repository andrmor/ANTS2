#ifndef AOUTWIN_SI_H
#define AOUTWIN_SI_H

#include "ascriptinterface.h"

#include <QVariant>

class MainWindow;

class AOutWin_SI : public AScriptInterface
{
  Q_OBJECT

public:
  AOutWin_SI(MainWindow* MW);
  ~AOutWin_SI(){}

public slots:
  void ShowOutputWindow(bool flag = true, int tab = -1);
  void Show();
  void Hide();

  void SetWindowGeometry(QVariant xywh);

private:
  MainWindow* MW;
};

#endif // AOUTWIN_SI_H
