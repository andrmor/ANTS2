#ifndef WINDOWNAVIGATORCLASS_H
#define WINDOWNAVIGATORCLASS_H

#include "aguiwindow.h"

class MainWindow;

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

  void ResetAllProgressBars();

  bool isMainChangeExplicitlyRequested() {return MainChangeExplicitlyRequested;}
  void TriggerHideButton();
  void TriggerShowButton();

  void BusyOn();
  void BusyOff(bool fShowTime = false);

  void DisableAllButGraphWindow(bool trueStart_falseStop);
  
public slots:
  void HideWindowTriggered(const QString & w);
  void ShowWindowTriggered(const QString & w);
  void on_pbMaxAll_clicked();
  void ChangeGuiBusyStatus(bool flag);
  void setProgress(int percent);

private slots:   
   void on_pbMinAll_clicked();
   void on_pbMain_clicked();
   void on_pbDetector_clicked();
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
  MainWindow * MW = nullptr;
  Ui::WindowNavigatorClass *ui = nullptr;
  QTime * time = nullptr;

  QString LastProcessedWindow;
  bool DisableBSupdate = false;
  bool MainChangeExplicitlyRequested = false;

  bool MainOn = false;
  bool DetectorOn = false;
  bool ReconOn = false;
  bool OutOn = false;
  bool MatOn = false;
  bool ExamplesOn = false;
  bool LRFon = false;
  bool NewLRFon = false;
  bool GeometryOn = false;
  bool GraphOn = false;
  bool ScriptOn = false;
  bool PythonScriptOn = false;

private:
  void updateButtons();
};

#endif // WINDOWNAVIGATORCLASS_H
