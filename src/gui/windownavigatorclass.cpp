#include "windownavigatorclass.h"
#include "ui_windownavigatorclass.h"
#include "mainwindow.h"
#include "outputwindow.h"
#include "reconstructionwindow.h"
#include "materialinspectorwindow.h"
#include "lrfwindow.h"
//#include "alrfwindow.h"
#include "graphwindowclass.h"
#include "geometrywindowclass.h"
#include "exampleswindow.h"
#include "gainevaluatorwindowclass.h"
#include "aglobalsettings.h"
#include "ascriptwindow.h"
#include "detectoraddonswindow.h"

#include <QTime>
#include <QDebug>

WindowNavigatorClass::WindowNavigatorClass(QWidget *parent, MainWindow *mw) :
    AGuiWindow("navi", parent),
    MW(mw),
    ui(new Ui::WindowNavigatorClass)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());

    Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
    windowFlags |= Qt::WindowCloseButtonHint;
    windowFlags |= Qt::WindowStaysOnTopHint;
    windowFlags |= Qt::Tool;
    this->setWindowFlags( windowFlags );

    time = new QTime();
}

WindowNavigatorClass::~WindowNavigatorClass()
{
    delete time; time = nullptr;
    delete ui;
}

void WindowNavigatorClass::setProgress(int percent)
{
    ui->pb->setValue(percent);
}

void WindowNavigatorClass::HideWindowTriggered(const QString & w)
{
    if (MW->ShutDown) return;
    if (DisableBSupdate) return;

    //qDebug() << "WinNav: hide win"<<w;

    if      (w == "main")     MainOn = false;
    else if (w == "detector") DetectorOn = false;
    else if (w == "recon")    ReconOn = false;
    else if (w == "out")      OutOn = false;
    else if (w == "mat")      MatOn = false;
    else if (w == "examples") ExamplesOn = false;
    else if (w == "lrf")      LRFon = false;
    else if (w == "newLrf")   NewLRFon = false;
    else if (w == "geometry") GeometryOn = false;
    else if (w == "graph")    GraphOn = false;
    else if (w == "script")   ScriptOn = false;
    else if (w == "python")   PythonScriptOn = false;

    updateButtons();
}

void WindowNavigatorClass::ShowWindowTriggered(const QString & w)
{
    if (MW->ShutDown) return;

    if      (w == "main")       MainOn = true;
    else if (w == "detector")   DetectorOn = true;
    else if (w == "recon")      ReconOn = true;
    else if (w == "out")        OutOn = true;
    else if (w == "mat")        MatOn = true;
    else if (w == "examples")   ExamplesOn = true;
    else if (w == "lrf")        LRFon = true;
    else if (w == "newLrf")     NewLRFon = true;
    else if (w == "geometry")   GeometryOn = true;
    else if (w == "graph")      GraphOn = true;
    else if (w == "script")     ScriptOn = true;
    else if (w == "python")     PythonScriptOn = true;

    updateButtons();
}

void WindowNavigatorClass::updateButtons()
{
    ui->pbMain->setChecked(MainOn);
    ui->pbDetector->setChecked(DetectorOn);
    ui->pbRecon->setChecked(ReconOn);
    ui->pbOut->setChecked(OutOn);
    ui->pbMaterials->setChecked(MatOn);
    ui->pbExamples->setChecked(ExamplesOn);
    ui->pbLRF->setChecked(LRFon);
    ui->pbNewLRF->setChecked(NewLRFon);
    ui->pbGeometry->setChecked(GeometryOn);
    ui->pbGraph->setChecked(GraphOn);
    ui->pbScript->setChecked(ScriptOn);
}

void WindowNavigatorClass::ResetAllProgressBars()
{
    MW->SetProgress(0);
    setProgress(0);
    MW->Rwindow->SetProgress(0);
    MW->lrfwindow->SetProgress(0);
}

void WindowNavigatorClass::TriggerHideButton()
{
    on_pbMinAll_clicked();
}

void WindowNavigatorClass::TriggerShowButton()
{
    on_pbMaxAll_clicked();
}

void WindowNavigatorClass::BusyOn()
{  
  MW->stopRootUpdate(); //--//

  MW->onBusyOn();
  MW->DAwindow->setEnabled(false);
  MW->Rwindow->onBusyOn();
  MW->ScriptWindow->onBusyOn();
  if (MW->PythonScriptWindow) MW->PythonScriptWindow->onBusyOn();
  MW->lrfwindow->onBusyOn();
  MW->MIwindow->setEnabled(false);
  MW->ELwindow->setEnabled(false);
  if (MW->GainWindow) MW->GainWindow->setEnabled(false);
  MW->GraphWindow->OnBusyOn(); //setEnabled(false);
  MW->GeometryWindow->onBusyOn();
  MW->Owindow->setEnabled(false);

  emit BusyStatusChanged(true);

  time->restart();

  qApp->processEvents();
}

void WindowNavigatorClass::BusyOff(bool fShowTime)
{
  int runtime = time->elapsed(); //gets the runtime in ms
  if (fShowTime)
  {
      QString str;
      str.setNum(runtime);
      MW->Owindow->OutText("Elapsed time: " + str + " ms\n");
  }

  MW->onBusyOff();
  MW->DAwindow->setEnabled(true);
  MW->Rwindow->onBusyOff();         //setEnabled(true);
  MW->ScriptWindow->onBusyOff();
  if (MW->PythonScriptWindow) MW->PythonScriptWindow->onBusyOff();
  MW->lrfwindow->onBusyOff();
  MW->MIwindow->setEnabled(true);
  MW->ELwindow->setEnabled(true);
  if (MW->GainWindow) MW->GainWindow->setEnabled(true);
  MW->GraphWindow->OnBusyOff();     //setEnabled(true);
  MW->GeometryWindow->onBusyOff();
  MW->Owindow->setEnabled(true);

  emit BusyStatusChanged(false);

  MW->startRootUpdate(); //--//
}

void WindowNavigatorClass::DisableAllButGraphWindow(bool trueStart_falseStop)
{
    MW->setEnabled(!trueStart_falseStop);
    MW->Rwindow->setEnabled(!trueStart_falseStop);
    MW->lrfwindow->setEnabled(!trueStart_falseStop);
    MW->MIwindow->setEnabled(!trueStart_falseStop);
    MW->ELwindow->setEnabled(!trueStart_falseStop);
    MW->GeometryWindow->setEnabled(!trueStart_falseStop);
    MW->Owindow->setEnabled(!trueStart_falseStop);
    MW->ScriptWindow->setEnabled(!trueStart_falseStop);
}

void WindowNavigatorClass::on_pbMaxAll_clicked()
{
  activateWindow();
  //qDebug()<<"---MAX ALL---";

  MainChangeExplicitlyRequested = true;

  if (MainOn)
    {
      MW->showNormal();
      MW->raise();
    }
  if (DetectorOn)
  {
      MW->DAwindow->showNormal();
      MW->DAwindow->raise();
  }
  if (ReconOn)
    {
      MW->Rwindow->showNormal();
      MW->Rwindow->raise();
    }
  if (OutOn)
    {
      MW->Owindow->showNormal();
      MW->Owindow->raise();
    }
  if (MatOn)
    {
      MW->MIwindow->showNormal();
      MW->MIwindow->raise();
    }
  if (ExamplesOn)
    {
      MW->ELwindow->showNormal();
      MW->ELwindow->raise();
    }
  if (LRFon)
    {
      MW->lrfwindow->showNormal();
      MW->lrfwindow->raise();
    }
  if (GeometryOn)
    {
      MW->GeometryWindow->showNormal();
      MW->GeometryWindow->raise();
      MW->GeometryWindow->ShowGeometry();
      MW->GeometryWindow->DrawTracks();
    }
  if (GraphOn)
    {
      MW->GraphWindow->showNormal();
      MW->GraphWindow->raise();
      MW->GraphWindow->UpdateRootCanvas();
    }
  if (ScriptOn)
    if (MW->ScriptWindow)
    {
      MW->ScriptWindow->showNormal();
      MW->ScriptWindow->raise();
    }
  if (PythonScriptOn)
    if (MW->PythonScriptWindow)
    {
      MW->PythonScriptWindow->showNormal();
      MW->PythonScriptWindow->raise();
    }

  MainChangeExplicitlyRequested = false;
}

void WindowNavigatorClass::ChangeGuiBusyStatus(bool flag)
{
    if (flag) BusyOn();
    else BusyOff();
}

void WindowNavigatorClass::on_pbMinAll_clicked()
{
  MainChangeExplicitlyRequested = true;
  DisableBSupdate = true;

  QVector<QMainWindow*> vec;

  vec << MW->DAwindow<<MW->Owindow<<MW->Rwindow<<MW->lrfwindow<<MW->MIwindow<<MW->ELwindow<<MW->GraphWindow<<MW->GeometryWindow<<MW->ScriptWindow;
  if (MW->PythonScriptWindow) vec << MW->PythonScriptWindow;
  if (MW->GenScriptWindow) vec <<MW->GenScriptWindow;
  if (MW->GainWindow) vec << MW->GainWindow;
  for (QMainWindow * win : vec) win->hide();

  MW->showMinimized();

  DisableBSupdate = false;
  MainChangeExplicitlyRequested = false;
}

void WindowNavigatorClass::on_pbMain_clicked()
{
   MainChangeExplicitlyRequested = true;
   if ( MW->isHidden() || MW->isMinimized() || (!MainOn) )
     {
       MW->showNormal();
       MW->raise();
     }
   else MW->showMinimized();//MW->hide();
   MainChangeExplicitlyRequested = false;
}

void WindowNavigatorClass::on_pbDetector_clicked()
{
    if ( MW->DAwindow->isHidden() || MW->DAwindow->isMinimized() || (!DetectorOn) )
    {
        MW->DAwindow->showNormal();
        MW->DAwindow->raise();
    }
    else MW->DAwindow->hide();
}

void WindowNavigatorClass::on_pbRecon_clicked()
{
  if ( MW->Rwindow->isHidden() || MW->Rwindow->isMinimized() || (!ReconOn) )
    {
      MW->Rwindow->showNormal();
      MW->Rwindow->raise();
    }
  else MW->Rwindow->hide();
}

void WindowNavigatorClass::on_pbOut_clicked()
{
  if ( MW->Owindow->isHidden() || MW->Owindow->isMinimized() || (!OutOn) )
    {
      MW->Owindow->showNormal();
      MW->Owindow->raise();
    }
  else MW->Owindow->hide();
}

void WindowNavigatorClass::on_pbLRF_clicked()
{
  if ( MW->lrfwindow->isHidden() || MW->lrfwindow->isMinimized() || (!LRFon) )
    {
      MW->lrfwindow->showNormal();
      MW->lrfwindow->raise();
    }
  else MW->lrfwindow->hide();
}

void WindowNavigatorClass::on_pbNewLRF_clicked()
{

}

void WindowNavigatorClass::on_pbGeometry_clicked()
{
  if ( MW->GeometryWindow->isHidden() || MW->GeometryWindow->isMinimized() || (!GeometryOn) )
    {
      MW->GeometryWindow->showNormal();
      MW->GeometryWindow->raise();
      MW->GeometryWindow->ShowGeometry();
      MW->GeometryWindow->DrawTracks();
    }
  else MW->GeometryWindow->hide();
}

void WindowNavigatorClass::on_pbGraph_clicked()
{
  if ( MW->GraphWindow->isHidden() || MW->GraphWindow->isMinimized() || (!GraphOn) )
    {
      MW->GraphWindow->showNormal();
      MW->GraphWindow->raise();
    }
  else MW->GraphWindow->hide();
}

void WindowNavigatorClass::on_pbMaterials_clicked()
{
  if ( MW->MIwindow->isHidden() || MW->MIwindow->isMinimized() || (!MatOn) )
    {
      MW->MIwindow->showNormal();
      MW->MIwindow->raise();
    }
  else MW->MIwindow->hide();  
}

void WindowNavigatorClass::on_pbExamples_clicked()
{
  if ( !MW->ELwindow->isVisible() || MW->ELwindow->isHidden() || MW->ELwindow->isMinimized() || (!ExamplesOn) )
    {
      MW->ELwindow->showNormal();
      MW->ELwindow->raise();
      MW->ELwindow->activateWindow();
    }
  else MW->ELwindow->hide();
}

void WindowNavigatorClass::on_pbScript_clicked()
{
    if ( MW->ScriptWindow->isHidden() || MW->ScriptWindow->isMinimized() || (!ScriptOn) )
      {
        MW->ScriptWindow->showNormal();
        MW->ScriptWindow->raise();
      }
    else MW->ScriptWindow->hide();
}

void WindowNavigatorClass::closeEvent(QCloseEvent *)
{
    MW->showNormal();
}

