#ifndef WINDOWNAVIGATORCLASS_H
#define WINDOWNAVIGATORCLASS_H

#include "aguiwindow.h"

class MainWindow;

#ifdef Q_OS_WIN32
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

namespace Ui {
  class WindowNavigatorClass;
}

class QTime;

class WindowNavigatorClass : public AGuiWindow
{
  Q_OBJECT
  
public:
  explicit WindowNavigatorClass(QWidget *parent, MainWindow *mw);
  ~WindowNavigatorClass();

  void SetupWindowsTaskbar(); //Windows only

  void ResetAllProgressBars();

  bool isMainChangeExplicitlyRequested() {return MainChangeExplicitlyRequested;}
  void TriggerHideButton();
  void TriggerShowButton();

  void BusyOn();
  void BusyOff(bool fShowTime = false);

  void DisableAllButGraphWindow(bool trueStart_falseStop);

#ifdef Q_OS_WIN32
  QWinTaskbarButton *taskButton;
  QWinTaskbarProgress *taskProgress;
#endif
  
public slots:
  void HideWindowTriggered(QString w);
  void ShowWindowTriggered(QString w);  
  void on_pbMaxAll_clicked();
  void ChangeGuiBusyStatus(bool flag);
  void setProgress(int percent);

private slots:   
   void on_pbMinAll_clicked();
   void on_pbMain_clicked();
   void on_pbRecon_clicked();
   void on_pbOut_clicked();
   void on_pbLRF_clicked();
   void on_pbNewLRF_clicked();
   void on_pbGeometry_clicked();
   void on_pbGraph_clicked();
   void on_pbMaterials_clicked();
   void on_pbExamples_clicked();   
   void on_pbScript_clicked();

protected:
   void closeEvent(QCloseEvent *);

signals:
   void BusyStatusChanged(bool);

private:
  Ui::WindowNavigatorClass *ui;
  MainWindow* MW;

  QTime* time;

  QString LastProcessedWindow;
  bool DisableBSupdate;
  bool MainOn, ReconOn, OutOn, LRFon, newLRFon, MatOn, ExamplesOn, GeometryOn, GraphOn, ScriptOn, PythonScriptOn;

  bool MainChangeExplicitlyRequested;
};

#endif // WINDOWNAVIGATORCLASS_H
