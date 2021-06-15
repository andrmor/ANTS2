//ANTS2 modules and windows
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "apmhub.h"
#include "materialinspectorwindow.h"
#include "reconstructionwindow.h"
#include "outputwindow.h"
#include "windownavigatorclass.h"
#include "exampleswindow.h"
#include "graphwindowclass.h"
#include "geometrywindowclass.h"
#include "lrfwindow.h"
#include "aglobalsettings.h"
#include "detectoraddonswindow.h"
#include "detectorclass.h"
#include "ajsontools.h"
#include "amessage.h"
#include "aconfiguration.h"
#include "ascriptwindow.h"
#include "apmgroupsmanager.h"
#include "guiutils.h"
#include "alrfwindow.h"
#include "aremotewindow.h"

//Qt
#include <QFileDialog>
#include <QInputDialog>
#include <QDebug>
#include <QWindow>
#include <QMessageBox>
#include <QDateTime>

void MainWindow::on_actionWindow_navigator_triggered()
{
  if (WindowNavigator)
    {
      WindowNavigator->showNormal();
      WindowNavigator->activateWindow();
    }
  else message("error - winnavigator was deleted", this);
}

void MainWindow::on_actionReconstructor_triggered()
{
  Rwindow->showNormal();
  Rwindow->raise();
  Rwindow->activateWindow();
}

void MainWindow::on_actionOutput_triggered()
{
  Owindow->showNormal();
  Owindow->raise();
  Owindow->activateWindow();
}

void MainWindow::on_actionLRF_triggered()
{
  lrfwindow->showNormal();
  lrfwindow->raise();
  lrfwindow->activateWindow();
}

void MainWindow::on_actionMaterial_inspector_window_triggered()
{
  MIwindow->showNormal();
  MIwindow->raise();
  MIwindow->activateWindow();
}

void MainWindow::on_actionGeometry_triggered()
{
  GeometryWindow->showNormal();
  GeometryWindow->raise();
  GeometryWindow->activateWindow();

  GeometryWindow->ShowGeometry(true, false);
  GeometryWindow->ShowTracksAndMarkers();
}

void MainWindow::on_actionGraph_triggered()
{
  if (!GraphWindow) return;
  GraphWindow->showNormal();
  GraphWindow->raise();
  GraphWindow->activateWindow();
}

void MainWindow::on_actionExamples_triggered()
{
  ELwindow->showNormal();
  ELwindow->raise();
  ELwindow->activateWindow();
}

void MainWindow::on_actionGain_evaluation_triggered()
{
  Rwindow->ShowGainWindow();
}

void MainWindow::on_actionReset_position_of_windows_triggered()
{
   QList<QMainWindow*> list;
   list << this<<Rwindow<<Owindow<<lrfwindow<<MIwindow<<WindowNavigator<<ELwindow<<DAwindow<<ScriptWindow<<newLrfWindow;
   foreach(QMainWindow *w, list)
     {
       w->move(20,20);
       w->showNormal();
       w->raise();
     }
   Owindow->resize(524, 385);

   if (GeometryWindow)
     {
       GeometryWindow->setGeometry(20, 20, 600, 600);
       GeometryWindow->showNormal();
       GeometryWindow->raise();
     }

   if (GraphWindow)
     {
       GraphWindow->setGeometry(20, 20, 700, 500);
       GraphWindow->showNormal();
       GraphWindow->raise();
     }

   if (PythonScriptWindow)
   {
       PythonScriptWindow->setGeometry(20, 20, 700, 500);
       PythonScriptWindow->showNormal();
       PythonScriptWindow->raise();
   }

   bOptOvDialogPositioned = false;

   OvTesterSettings["PositionX"] = -1;
   OvTesterSettings["PositionY"] = -1;
}

void addWindow(QString name, QMainWindow* w, QJsonObject &json)
{
  QJsonObject js;
  bool fMaxi = w->isMaximized();
  js["maximized"] = fMaxi;
  js["vis"] = w->isVisible();

  if (fMaxi)
    {
      w->showNormal();
      qApp->processEvents();
    }
  js["x"] = w->x();
  js["y"] = w->y();
  js["w"] = w->width();
  js["h"] = w->height();

  json[name] = js;
}

void MainWindow::on_actionSave_position_and_stratus_of_all_windows_triggered()
{
  QJsonObject json;

  QVector<AGuiWindow*> vec;
  vec << this << Rwindow << ELwindow << Owindow << DAwindow << GeometryWindow << GraphWindow
      <<    lrfwindow << MIwindow << ScriptWindow << WindowNavigator << RemoteWindow << newLrfWindow;
  if (PythonScriptWindow) vec << PythonScriptWindow;
  for (AGuiWindow * w : vec)
      w->writeGeomToJson(json);

  if (GenScriptWindow)
  {
      ScriptWinX = GenScriptWindow->x();
      ScriptWinY = GenScriptWindow->y();
      ScriptWinW = GenScriptWindow->width();
      ScriptWinH = GenScriptWindow->height();
  }
  QJsonObject js;
  js["x"] = ScriptWinX;
  js["y"] = ScriptWinY;
  js["w"] = ScriptWinW;
  js["h"] = ScriptWinH;
  json["LocalScript"] = js;

  QJsonObject jsOD;
  jsOD["x"] = OptOvDialogPosition.x();
  jsOD["y"] = OptOvDialogPosition.y();
  jsOD["w"] = OptOvDialogSize.width();
  jsOD["h"] = OptOvDialogSize.height();
  json["OptOvDialog"] = jsOD;

  json["OptOvTester"] = OvTesterSettings;

  QString fileName = GlobSet.ConfigDir + "/WindowConfig.ini";
  bool bOK = SaveJsonToFile(json, fileName);
  if (!bOK) message("Failed to save json to file: "+fileName, this);
}

void MainWindow::on_actionLoad_positions_and_status_of_all_windows_triggered()
{
  QString fileName = GlobSet.ConfigDir + "/WindowConfig.ini";
  if (QFile(fileName).exists())
    {
      QJsonObject json;
      bool ok = LoadJsonFromFile(json, fileName);
      if (!ok)
      {
          qDebug() << "Config of window positions/sizes: empty json";
          return;
      }

      QVector<AGuiWindow*> vec;
      vec << this << Rwindow << ELwindow << Owindow << DAwindow << GeometryWindow << GraphWindow
          <<    lrfwindow << MIwindow << ScriptWindow << WindowNavigator << RemoteWindow << newLrfWindow;
      if (PythonScriptWindow) vec << PythonScriptWindow;
      for (AGuiWindow * w : vec)
          w->readGeomFromJson(json);

      if (json.contains("LocalScript"))
      {
          QJsonObject js = json["Script"].toObject();
          parseJson(js, "x", ScriptWinX);
          parseJson(js, "y", ScriptWinY);
          parseJson(js, "w", ScriptWinW);
          parseJson(js, "h", ScriptWinH);
          if (GenScriptWindow) recallGeometryOfLocalScriptWindow();
      }

      if (json.contains("OptOvDialog"))
      {
          QJsonObject jsOD = json["OptOvDialog"].toObject();
          int x, y, w, h;
          parseJson(jsOD, "x", x);
          parseJson(jsOD, "y", y);
          OptOvDialogPosition = QPoint(x, y);
          parseJson(jsOD, "w", w);
          parseJson(jsOD, "h", h);
          OptOvDialogSize = QSize(w, h);
          bOptOvDialogPositioned = true;
      }

      parseJson(json, "OptOvTester", OvTesterSettings);
    }
}

void MainWindow::SetProgress(int val)
{
  ui->prScan->setValue(val);
}

void MainWindow::SetMultipliersUsingGains(QVector<double> Gains)
{
    if (Gains.size() < PMs->count())
      {
        message("Gain data corrupted!", this);
        return;
      }
  for (int ipm=0; ipm<PMs->count(); ipm++)
    {
      double gain = Gains.at(ipm);
      if (fabs(gain)>1.0e-20) PMs->at(ipm).PreprocessingMultiply = 1.0/gain;
    }
  int ipm = ui->sbPreprocessigPMnumber->value();
  ui->lePreprocessingMultiply->setText( QString::number( PMs->at(ipm).PreprocessingMultiply) );

  on_pbUpdatePreprocessingSettings_clicked();
}

void MainWindow::SetMultipliersUsingChPhEl(QVector<double> ChPerPhEl)
{
    if (ChPerPhEl.size() < PMs->count())
      {
        message("Channels per photoelectron data is not defined or not complete!", this);
        return;
      }
    for (int ipm=0; ipm<PMs->count(); ipm++)
        PMs->at(ipm).PreprocessingMultiply /= ChPerPhEl.at(ipm);

    int ipm = ui->sbPreprocessigPMnumber->value();
    ui->lePreprocessingMultiply->setText( QString::number( PMs->at(ipm).PreprocessingMultiply) );

    on_pbUpdatePreprocessingSettings_clicked();
}

void MainWindow::CorrectPreprocessingAdds(QVector<double> Pedestals)
{
  if (Pedestals.size() < PMs->count())
    {
      message("Size not valid!", this);
      return;
    }
  for (int ipm=0; ipm<PMs->count(); ipm++)
      PMs->at(ipm).PreprocessingAdd -= Pedestals.at(ipm);

  int ipm = ui->sbPreprocessigPMnumber->value();
  ui->ledPreprocessingAdd->setText( QString::number( PMs->at(ipm).PreprocessingAdd) );

  on_pbUpdatePreprocessingSettings_clicked();
}

void MainWindow::on_actionSave_configuration_triggered()
{
  QFileDialog fileDialog;
  QString fileName = fileDialog.getSaveFileName(this, "Save configuration", GlobSet.LastOpenDir, "Json files (*.json)");
  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
  QFileInfo file(fileName);
  if(file.suffix().isEmpty()) fileName += ".json";
  ELwindow->SaveConfig(fileName);
}

void MainWindow::on_actionLoad_configuration_triggered()
{
  QFileDialog fileDialog;
  QString fileName = fileDialog.getOpenFileName(this,"Load configuration", GlobSet.LastOpenDir, "All files (*.*)");
  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
  bool bOK = Config->LoadConfig(fileName);
  if (!bOK) message(Config->ErrorString, this);
  if (GeometryWindow->isVisible()) GeometryWindow->ShowGeometry();
}

void MainWindow::on_actionNew_detector_triggered()
{
  QMessageBox msgBox(this);
  msgBox.setText("Start new detector?");
  msgBox.setInformativeText("All unsaved changes to the configuration will be lost!");
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
  msgBox.setDefaultButton(QMessageBox::Cancel);
  msgBox.setIcon(QMessageBox::Question);
  int ret = msgBox.exec();
  if (ret == QMessageBox::Cancel) return;

  Config->LoadConfig(GlobSet.ExamplesDir + "/Simplest.json");
  if (ELwindow->isVisible()) ELwindow->hide();
  if (GeometryWindow->isVisible()) GeometryWindow->ShowGeometry(false);
}

void MainWindow::setFontSizeAllWindows(int size)
{
  QFont font = this->font();
  font.setPointSize(size);

  QList<QMainWindow*> wins;
  wins << (QMainWindow*)this << (QMainWindow*)Rwindow << (QMainWindow*)Owindow << (QMainWindow*)MIwindow
       << (QMainWindow*)lrfwindow << (QMainWindow*)WindowNavigator << (QMainWindow*)GeometryWindow
       << (QMainWindow*)GraphWindow << (QMainWindow*)ELwindow << (QMainWindow*)DAwindow
       << (QMainWindow*)GlobSetWindow << (QMainWindow*)CheckUpWindow << (QMainWindow*)RemoteWindow;
  if (GainWindow) wins << (QMainWindow*)GainWindow;
  if (PythonScriptWindow) wins << (QMainWindow*)PythonScriptWindow;

  for (int i=0; i<wins.size(); i++) wins[i]->setFont(font);
}

void MainWindow::on_actionGlobal_script_triggered()
{
    on_actionScript_window_triggered();
}

void MainWindow::on_actionScript_window_triggered()
{
    ScriptWindow->show();
    ScriptWindow->raise();
    ScriptWindow->activateWindow();
}

void MainWindow::on_actionQuick_save_1_triggered()
{
    ELwindow->QuickSave(1);
}

void MainWindow::on_actionQuick_save_2_triggered()
{
    ELwindow->QuickSave(2);
}

void MainWindow::on_actionQuick_save_3_triggered()
{
    ELwindow->QuickSave(3);
}

void MainWindow::on_actionQuick_load_1_triggered()
{
    ELwindow->QuickLoad(1, this);
}

void MainWindow::on_actionQuick_load_2_triggered()
{
    ELwindow->QuickLoad(2, this);
}

void MainWindow::on_actionQuick_load_3_triggered()
{
    ELwindow->QuickLoad(3, this);
}

void MainWindow::on_actionLoad_last_config_triggered()
{
    ELwindow->QuickLoad(0, this);
}

void MainWindow::on_actionQuick_save_1_hovered()
{
    ui->actionQuick_save_1->setToolTip(ELwindow->getQuickSlotMessage(1));
}

void MainWindow::on_actionQuick_save_2_hovered()
{
    ui->actionQuick_save_2->setToolTip(ELwindow->getQuickSlotMessage(2));
}

void MainWindow::on_actionQuick_save_3_hovered()
{
    ui->actionQuick_save_3->setToolTip(ELwindow->getQuickSlotMessage(3));
}

void MainWindow::on_actionQuick_load_1_hovered()
{
    ui->actionQuick_load_1->setToolTip(ELwindow->getQuickSlotMessage(1));
}

void MainWindow::on_actionQuick_load_2_hovered()
{
    ui->actionQuick_load_2->setToolTip(ELwindow->getQuickSlotMessage(2));
}

void MainWindow::on_actionQuick_load_3_hovered()
{
    ui->actionQuick_load_3->setToolTip(ELwindow->getQuickSlotMessage(3));
}

void MainWindow::on_actionLoad_last_config_hovered()
{
    ui->actionLoad_last_config->setToolTip("Load config on last exit/sim/reconstruct\n" + ELwindow->getQuickSlotMessage(0));
}

void MainWindow::on_actionOpen_Python_window_triggered()
{
    if (PythonScriptWindow)
    {
        PythonScriptWindow->showNormal();
        PythonScriptWindow->raise();
        PythonScriptWindow->activateWindow();
    }
    else
    {
        message("Python scripting was not compiled, see ants2.pro", this);
    }
}
