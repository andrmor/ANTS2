#include "lrfwindow.h"
#include "ui_lrfwindow.h"
#include "mainwindow.h"
#include "apmhub.h"
#include "detectorclass.h"
#include "graphwindowclass.h"
#include "sensorlrfs.h"
#include "alrfmoduleselector.h"
#include "geometrywindowclass.h"
#include "outputwindow.h"
#include "eventsdataclass.h"
#include "aglobalsettings.h"
#include "ajsontools.h"
#include "apositionenergyrecords.h"
#include "windownavigatorclass.h"
#include "reconstructionwindow.h"
#include "jsonparser.h"
#include "amessage.h"
#include "aconfiguration.h"
#include "ascriptwindow.h"
#include "tmpobjhubclass.h"
#include "apmgroupsmanager.h"
#include "alrfmouseexplorer.h"
#include "alrfdraw.h"

//Qt
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>

//ROOT
#include "TStyle.h"
#include "TMath.h"
#include "TTree.h"
#include "TFile.h"
#include "TF1.h"
#include "TAxis.h"
#include "TGraph.h"
#include "TGraph2D.h"

LRFwindow::LRFwindow(QWidget *parent, MainWindow *mw, EventsDataClass *eventsDataHub) :
  QMainWindow(parent),
  ui(new Ui::LRFwindow)
{
  MW = mw;
  EventsDataHub = eventsDataHub;
  SensLRF = MW->Detector->LRFs->getOldModule();
  PMs = MW->Detector->PMs;

  tmpIgnore = false;
  secondIter = -1;
  fOneClick = false;

  ui->setupUi(this);
  this->setFixedSize(this->width(), this->height());

  Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
  windowFlags |= Qt::WindowCloseButtonHint;
  this->setWindowFlags( windowFlags );

  StopSignal = false;

  ui->pbUpdateGUI->setVisible(false);
  ui->pbUpdateLRFmakeJson->setVisible(false);
  ui->pbUpdateHistory->setVisible(false);
  //ui->fSingleGroup->setEnabled(false);

  ui->lwLRFhistory->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui->lwLRFhistory, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenuHistory(const QPoint &)));

  QObject::connect(SensLRF, SIGNAL(SensorLRFsReadySignal(bool)), this, SLOT(LRF_ModuleReadySlot(bool)));
  QObject::connect(SensLRF, SIGNAL(ProgressReport(int)), this, SLOT(onProgressReportReceived(int)));

#ifndef NEWFIT
  ui->frAdvancedLrfConstrains->setEnabled(false);

  ui->cbForceNonNegative->setChecked(false);
  ui->cbForceNonIncreasingInR->setChecked(false);
  ui->cobForceInZ->setCurrentIndex(0);
  ui->cbForceTopDown->setChecked(false);
#endif

  LRFwindow::on_pbUpdateGUI_clicked(); //update GUI to set enable/visible/index status
  LRFwindow::on_pbShrink_clicked();
  ui->fSingleGroup->setEnabled(false);
  updateWarningForEnergyScan();
}

LRFwindow::~LRFwindow()
{
  delete ui;
}

void LRFwindow::SetProgress(int val)
{
  ui->pbarLRF->setValue(val);
}

void LRFwindow::onBusyOn()
{
  ui->fAll->setEnabled(false);
}

void LRFwindow::onBusyOff()
{
  ui->fAll->setEnabled(true);
}

void LRFwindow::showLRF()
{
    ALrfDraw lrfd(MW->Detector->LRFs, true, EventsDataHub, PMs, MW->GraphWindow);

    QJsonObject json;
    json["DataSource"] =  ui->cb_data_selector->currentIndex(); //0 - sim, 1 - loaded
    json["PlotLRF"] = ui->cb_LRF->isChecked();
    json["PlotSecondaryLRF"] = ( (secondIter!=-1) && (secondIter < SensLRF->countIterations()) );
    json["PlotData"] = ui->cb_data->isChecked();
    json["PlotDiff"] = ui->cb_diff->isChecked();
    //json["FixedMin"] = ui->cbFixedMin->isChecked();
    //json["FixedMax"] = ui->cbFixedMax->isChecked();
    //json["Min"] = ui->ledFixedMin->text().toDouble();
    //json["Max"] = ui->ledFixedMax->text().toDouble();
    json["CurrentGroup"] = MW->Detector->PMgroups->getCurrentGroup();
    json["CheckZ"] = ui->cb3D_LRFs->isChecked();
    json["Z"] = ui->ledZcenter->text().toDouble();
    json["DZ"] = ui->ledZrange->text().toDouble();
    json["EnergyScaling"] = ui->cbEnergyScalling->isChecked();
    json["FunctionPointsX"] = MW->GlobSet.FunctionPointsX;
    json["FunctionPointsY"] = MW->GlobSet.FunctionPointsY;
    //json["Bins"] = ui->sbNumBins_2->value();
    //json["ShowNodes"] = ui->cbShowNodePositions->isChecked();

    QJsonObject js;
    js["SecondIteration"] = secondIter;
    json["ModuleSpecific"] = js;

    lrfd.DrawXY(ui->sbPMnoButons->value(), json);
}

void LRFwindow::on_pbStopLRFmake_clicked()
{
  StopSignal = true;
  ui->pbStopLRFmake->setText("stopping...");
  SensLRF->requestStop();
}

bool LRFwindow::event(QEvent *event)
{
  if (!MW->WindowNavigator) return QMainWindow::event(event);

  if (event->type() == QEvent::Hide) MW->WindowNavigator->HideWindowTriggered("lrf");
  if (event->type() == QEvent::Show) MW->WindowNavigator->ShowWindowTriggered("lrf");

  return QMainWindow::event(event);
}

void LRFwindow::on_pbUpdateGUI_clicked()
{
  if (tmpIgnore) return;

  ui->fPMgroups->setEnabled(ui->cbUseGroupping->isChecked());
  bool f3D = ui->cb3D_LRFs->isChecked();

  if (f3D) ui->swLRFtype->setCurrentIndex(1);
  else ui->swLRFtype->setCurrentIndex(0);

  bool fCompression = true;
  bool fEnableShowAxial3D = false;
  if (f3D)
    {
      if (ui->cob3Dtype->currentIndex() == 1) fCompression = false; //not for sliced XY
      else fEnableShowAxial3D = true;
    }
  else
    {
      if (ui->cob2Dtype->currentIndex() == 1) fCompression = false; //for XY cannot use compression
    }
  ui->fCompression->setVisible(fCompression);
  ui->frVsZ->setEnabled(fEnableShowAxial3D);

  // Nodes
  if (f3D)
    {
      //        qDebug()<<"3D type:"<< ui->cob3Dtype->currentIndex();
      if (ui->cob3Dtype->currentIndex() == 0)
        {
          // 3D axial
          ui->lbl_nodes_X->setText("R:");
          ui->lbl_nodes_Y->setText("Z:");
          ui->sbSplNodesY->setEnabled(true);
        }
      if (ui->cob3Dtype->currentIndex() == 1)
        {
          // 3D sliced
          ui->lbl_nodes_X->setText("X&Y:");
          ui->lbl_nodes_Y->setText("Z:");
          ui->sbSplNodesY->setEnabled(true);
        }
    }
  else
    {
      //       qDebug()<<"2D type:"<< ui->cob2Dtype->currentIndex();
      switch (ui->cob2Dtype->currentIndex())
        {
        case 0: // axial
          ui->lbl_nodes_X->setText("R:");
          ui->lbl_nodes_Y->setText("");
          ui->sbSplNodesY->setEnabled(false);
          break;
        case 1: // XY
          ui->lbl_nodes_X->setText("X:");
          ui->lbl_nodes_Y->setText("Y:");
          ui->sbSplNodesY->setEnabled(true);
          break;
        case 2: // polar
          ui->lbl_nodes_X->setText("R:");
          ui->lbl_nodes_Y->setText("Phi:");
          ui->sbSplNodesY->setEnabled(true);
          break;
        case 3: // composite
          ui->lbl_nodes_X->setText("R:");
          ui->lbl_nodes_Y->setText("X&Y:");
          ui->sbSplNodesY->setEnabled(true);          
          break;
        }
    }

  //In show, Z control - better to synchronize with actual LRF ***
  ui->fZ->setEnabled(f3D);
}

void LRFwindow::LRF_ModuleReadySlot(bool ready)
{
  //status label
  QString status;
  if (ready)
    {
      status = "Ready";
    }
  else
    {
      status = "Not Ready";
      if (SensLRF)
        if (ui->lwLRFhistory->count() != 0) LRFwindow::on_pbUpdateHistory_clicked();
    }

  ui->label_ready->setText(status);
  if (ready) ui->label_ready->setStyleSheet("QLabel { color : green; }");
  else ui->label_ready->setStyleSheet("QLabel { color : red; }");

  ui->pbSaveLRFs->setEnabled(ready); //save enabled
  ui->pbSaveIterations->setEnabled(ready); //save enabled
  ui->fPlot->setEnabled(ready); //Plot enabled  
}

void LRFwindow::on_pbSaveLRFs_clicked()
{
  LRFwindow::SaveLRFDialog(this);
}

void LRFwindow::SaveLRFDialog(QWidget* wid)
{    
  if (!SensLRF->isAllLRFsDefined())
    {
      message("Cannot save empty or incomplete LRF set!", wid);
      return;
    }

  QFileDialog *fileDialog = new QFileDialog;
  fileDialog->setDefaultSuffix("json");
  QString fileName = fileDialog->getSaveFileName(this, "Save LRF Block", MW->GlobSet.LastOpenDir, "json files(*.json);;all files(*.*)");
  if (fileName.isEmpty()) return;
  MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  int iCur = SensLRF->getCurrentIterIndex();
  if (iCur == -1)
    {
      message("There is no LRF data!", wid);
      return;
    }

  LRFwindow::SaveIteration(iCur, fileName);
}

void LRFwindow::SaveIteration(int iter, QString fileName)
{
  QFileInfo file(fileName);
  if(file.suffix().isEmpty()) fileName += ".json";
  QFile saveFile(fileName);
  if (!saveFile.open(QIODevice::WriteOnly))
    {
      message("Cannot open file "+fileName + " to save LRFs!");
      return;
    }

  QJsonObject json;
  if (!SensLRF->saveIteration(iter, json))
    {
      message(SensLRF->getLastError());
      saveFile.close();
      return;
    }

  QJsonDocument saveDoc(json);
  saveFile.write(saveDoc.toJson());
  saveFile.close();
}

void LRFwindow::on_pbSaveIterations_clicked()
{
  int numIter = SensLRF->countIterations();
  if (numIter == 0)
    {
      message("The iteration history is empty!");
      return;
    }

  QString fileName = QFileDialog::getSaveFileName(this, "Save all Iterations: provided name will be used to create a dir", MW->GlobSet.LastOpenDir, "json files(*.json);;all files(*.*)");
  if (fileName.isEmpty()) return;
  MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  QFileInfo fi(fileName);
  QString dir = fi.baseName();
  if (!QDir(fi.path()+"/"+dir).exists()) QDir().mkdir(fi.path()+"/"+dir);
  else
    {
      message("This directory already exists, please choose another name");
      return;
    }

  for (int iter=0; iter<numIter; iter++)
    {
      QString name = SensLRF->getIterationName(iter);
      name = fi.path()+"/"+dir+"/"+name;
      if (QFileInfo(name+".json").exists())
        {
          int index = 1;
          while (QFileInfo(name+QString::number(index)+".json").exists()) index++;
          name += QString::number(index);
        }
      name += ".json";

      LRFwindow::SaveIteration(iter, name);
    }  
}

void LRFwindow::on_pbLoadLRFs_clicked()
{
  LRFwindow::LoadLRFDialog(this);
}

void LRFwindow::LoadLRFDialog(QWidget* wid)
{
  QStringList fileNames = QFileDialog::getOpenFileNames(this, "Load LRF Block", MW->GlobSet.LastOpenDir, "JSON files (*.json)");
  if (fileNames.isEmpty()) return;
  MW->GlobSet.LastOpenDir = QFileInfo(fileNames.first()).absolutePath();

  for (int ifile=0; ifile<fileNames.size(); ifile++)
    {
      QString fileName = fileNames[ifile];
      QFile loadFile(fileName);
      if (!loadFile.open(QIODevice::ReadOnly))
        {
          message("Cannot open save file", wid);
          continue;
        }

      QByteArray saveData = loadFile.readAll();
      loadFile.close();

      QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
      QJsonObject json = loadDoc.object();
      if(!SensLRF->loadAll(json))
      {
          message("Failed to load LRFs: " + SensLRF->getLastError());
          continue;
      }
//      if (json.contains("LRF_window"))
//        {
//          QJsonObject a = json["LRF_window"].toObject();
//          if (!a.isEmpty()) LRFwindow::loadJSON(a);
//          else qDebug() << "LRF window settings were empty!";
//        }
    }

  //update GUI  
  LRFwindow::updateIterationIndication();
  LRFwindow::on_pbUpdateLRFmakeJson_clicked();
}

void LRFwindow::on_pbShow2D_clicked()
{  
  if (SensLRF->getIteration() == 0) return;  
  LRFwindow::showLRF();
  MW->GraphWindow->LastDistributionShown = "scatter";
}

void LRFwindow::on_sbPMnoButons_editingFinished()
{
  if (MW->GraphWindow->isVisible())
    {
      if (MW->GraphWindow->LastDistributionShown == "scatter") LRFwindow::showLRF();
      if (MW->GraphWindow->LastDistributionShown == "distrib") LRFwindow::drawRadial();
    }
}

void LRFwindow::on_sbPMnumberSpinButtons_valueChanged(int arg1)
{
  if (ui->sbPMnoButons->value() != arg1) ui->sbPMnoButons->setValue(arg1);
  if (arg1 > PMs->count()-1)
    {
      if (arg1 == 0) return;
      ui->sbPMnumberSpinButtons->setValue(0);
      return; //update on_change
    }

  if (tmpIgnore) return;
  if (MW->GraphWindow->isVisible())
    {
      if (MW->GraphWindow->LastDistributionShown == "scatter") LRFwindow::showLRF();
      if (MW->GraphWindow->LastDistributionShown == "distrib") LRFwindow::drawRadial();
    }
}

void LRFwindow::on_sbPMnoButons_valueChanged(int arg1)
{
  tmpIgnore = true;
  ui->sbPMnumberSpinButtons->setValue(arg1); //link to respect 0
  tmpIgnore = false;
}

void LRFwindow::writeToJson(QJsonObject &json) const
{
  //General
  json["DataSelector"] = ui->cb_data_selector->currentIndex();
  json["UseGrid"] = ui->cb_use_grid->isChecked();
  json["ForceZeroDeriv"] = ui->cbForceDerivToZeroInOrigin->isChecked();
  json["ForceNonNegative"] = ui->cbForceNonNegative->isChecked();
  json["ForceTopDown"] = ui->cbForceTopDown->isChecked();
  json["ForceNonIncreasingInR"] = ui->cbForceNonIncreasingInR->isChecked();
  json["ForceInZ"] = ui->cobForceInZ->currentIndex();
  json["StoreError"] = ui->cb_store_error->isChecked();
  json["UseEnergy"] = ui->cbEnergyScalling->isChecked();

  //Grouping
  bool fUseGrouping = ui->cbUseGroupping->isChecked();
  json["UseGroupping"] = fUseGrouping;
  if (fUseGrouping) json["GroupingOption"] = ui->cb_pmt_group_type->currentText();
  else              json["GroupingOption"] = "Individual";
  json["GrouppingType"] = ui->cb_pmt_group_type->currentIndex();
  json["AdjustGains"] = ui->cb_adj_gains->isChecked();

  //LRF type
  bool f3D = ui->cb3D_LRFs->isChecked();
  json["LRF_3D"] = f3D;
  int LRFtype;
  if (f3D) LRFtype = ui->cob3Dtype->currentIndex();
  else LRFtype = ui->cob2Dtype->currentIndex();
  json["LRF_type"] = LRFtype;

  //Compression properties
  json["LRF_compress"] = ui->cb_compress_r->isChecked();
  json["Compression_k"] = ui->led_compression_k->text().toDouble();
  json["Compression_r0"] = ui->led_compression_r0->text().toDouble();
  json["Compression_lam"] = ui->led_compression_lam->text().toDouble();

  //Number of nodes
  json["Nodes_x"] = ui->sbSplNodesX->value();
  json["Nodes_y"] = ui->sbSplNodesY->value();

  //Limit group making
  json["LimitGroup"] = ui->cbMakePMGroup->isChecked();
  json["GroupToMake"] = ui->sbMakePMgroup->value();
}

void LRFwindow::onRequestGuiUpdate()
{
  if ( MW->Config->JSON.contains("ReconstructionConfig") )
    {
      QJsonObject obj = MW->Config->JSON["ReconstructionConfig"].toObject();
      if (obj.contains("ReconstructionConfig"))
        {
          QJsonObject lrfopt = obj["LRFmakeJson"].toObject();
          loadJSON(lrfopt);
        }
    }
}

void LRFwindow::loadJSON(QJsonObject &json)
{
  tmpIgnore = true; //starting bulk update

  JsonParser parser(json);
  int version;
  parser.ParseObject("ANTS2 version", version);
  bool bTmp; int iTmp;

  parser.ParseObject("DataSelector", iTmp);
  ui->cb_data_selector->setCurrentIndex(iTmp);
  parser.ParseObject("UseGrid", bTmp);
  ui->cb_use_grid->setChecked(bTmp);
  parser.ParseObject("ForceZeroDeriv", bTmp);
  ui->cbForceDerivToZeroInOrigin->setChecked(bTmp);

  ui->cbForceNonNegative->setChecked(false);
  ui->cbForceNonIncreasingInR->setChecked(false);
  ui->cobForceInZ->setCurrentIndex(0);
  ui->cbForceTopDown->setChecked(false);
  JsonToCheckbox(json, "ForceNonNegative", ui->cbForceNonNegative);
  JsonToCheckbox(json, "ForceNonIncreasingInR", ui->cbForceNonIncreasingInR);
  JsonToCheckbox(json, "ForceTopDown", ui->cbForceTopDown);
  JsonToComboBox(json, "ForceInZ", ui->cobForceInZ);

  parser.ParseObject("StoreError", bTmp);
  ui->cb_store_error->setChecked(bTmp);
  parser.ParseObject("UseEnergy", bTmp);
  ui->cbEnergyScalling->setChecked(bTmp);

  parser.ParseObject("UseGroupping", bTmp);
  ui->cbUseGroupping->setChecked(bTmp);
  parser.ParseObject("GrouppingType", iTmp);
  if (iTmp>ui->cb_pmt_group_type->count()-1) iTmp = 0;
  if (bTmp) ui->cb_pmt_group_type->setCurrentIndex(iTmp);
  else ui->cb_pmt_group_type->setCurrentIndex(0);
  parser.ParseObject("AdjustGains", bTmp);
  ui->cb_adj_gains->setChecked(bTmp);

  bool f3D;
  parser.ParseObject("LRF_3D", f3D);
  ui->cb3D_LRFs->setChecked(f3D);
  parser.ParseObject("LRF_type", iTmp);
  if (f3D) ui->cob3Dtype->setCurrentIndex(iTmp);
  else ui->cob2Dtype->setCurrentIndex(iTmp);

  JsonToCheckbox(json, "LRF_compress", ui->cb_compress_r);
  JsonToLineEditDouble(json, "Compression_k", ui->led_compression_k);
  JsonToLineEditDouble(json, "Compression_r0", ui->led_compression_r0);
  JsonToLineEditDouble(json, "Compression_lam", ui->led_compression_lam);

  JsonToSpinBox(json, "Nodes_x", ui->sbSplNodesX);
  JsonToSpinBox(json, "Nodes_y", ui->sbSplNodesY);
  tmpIgnore = false;

  LRFwindow::on_pbUpdateGUI_clicked();
  LRFwindow::on_pbUpdateHistory_clicked();
}

void LRFwindow::on_pbShow1Ddistr_clicked()
{
  if (SensLRF->getIteration() == 0) return;
  LRFwindow::drawRadial();
  MW->GraphWindow->LastDistributionShown = "distrib";
}

void LRFwindow::on_cb_data_2_clicked()
{
  bool data = ui->cb_data_2->isChecked();
  bool dif  = ui->cb_diff_2->isChecked();

  if (data && dif) ui->cb_diff_2->setChecked(false);
  LRFwindow::on_pbShow1Ddistr_clicked();
}

void LRFwindow::on_cb_diff_2_clicked()
{
  bool data = ui->cb_data_2->isChecked();
  bool dif  = ui->cb_diff_2->isChecked();

  if (data && dif) ui->cb_data_2->setChecked(false);
  LRFwindow::on_pbShow1Ddistr_clicked();
}

bool LRFwindow::checkWatchdogs()
{
    const int minDataSize = 100;

    int dataScanRecon = ui->cb_data_selector->currentIndex(); //0 - Scan, 1 - Reconstr data
    QVector < QVector <float> > *events = &EventsDataHub->Events;

    int dataSize = events->size();
    if (dataSize == 0)
      {
        LastError = "Data is empty!";
        return false;
      }
    if (dataSize < minDataSize)
      {
        LastError = "Data set is too small!";
        return false;
      }
    if (EventsDataHub->countGoodEvents() < minDataSize)
      {
        LastError = "Data set is too small!";
        return false;
      }

    if (dataScanRecon == 0)
      {      
        if (EventsDataHub->isScanEmpty())
          {
            LastError = "Scan data are not available!";
            return false;
          }
      }
    else
      {       
        if (!EventsDataHub->isReconstructionReady())
          {
            LastError = "Reconstruction was not yet performed!";
            return false;
          }
      }

    bool fLimitGroup = ui->cbMakePMGroup->isChecked();
    int igrToMake = ui->sbMakePMgroup->value();
    if(fLimitGroup)
       {
         if (!SensLRF->getIteration())
           {
             LastError = "Current iteration is empty!";
             return false;
           }
         if (igrToMake >= SensLRF->countGroups())
           {
             LastError = "Group index out of bounds";
             return false;
           }
      }

    int type;
    bool f3D = ui->cb3D_LRFs->isChecked();
    if (f3D) type = ui->cob3Dtype->currentIndex() + 4; // *** absolute number!!!
    else type = ui->cob2Dtype->currentIndex();
    //  qDebug() << "LRF type: " << type;
    if (type == 2)
      {
        LastError = "Polar LRFs are not implemented in this version";
        return false;
      }
    return true;
}

void LRFwindow::on_pbMakeLRF_clicked()
{
  //checking that all necessary data is present and valid
  if (!LRFwindow::checkWatchdogs())
  {
      message(LastError, this);
      return;
  }

  MW->WindowNavigator->ResetAllProgressBars();
  MW->WindowNavigator->BusyOn();
  ui->pbStopLRFmake->setEnabled(true);
  qApp->processEvents();

  //packing settings
  LRFwindow::saveLRFmakeSettingsToJson();
  SaveJsonToFile(SensLRF->LRFmakeJson, "makeLRF.json");

  QString res = LRFwindow::MakeLRFs();
  if (!res.isEmpty()) message(res, this);

  ui->pbStopLRFmake->setEnabled(false);
  ui->pbStopLRFmake->setText("Stop");
  MW->WindowNavigator->BusyOff(false);
}

void LRFwindow::saveLRFmakeSettingsToJson()
{
    writeToJson(SensLRF->LRFmakeJson);
    MW->Config->UpdateLRFmakeJson();
}

QString LRFwindow::MakeLRFs()
{
  bool ok = SensLRF->makeLRFs(SensLRF->LRFmakeJson, EventsDataHub, PMs);

  LRFwindow::on_pbUpdateHistory_clicked(); //updating iteration history indication

  if (!ok) return SensLRF->getLastError();
  else return "";
}

void LRFwindow::on_pbAdjustOnlyGains_clicked()
{
    //checking that all necessary data is present and valid
    if (!LRFwindow::checkWatchdogs())
      {
        message(LastError, this);
        return;
      }

    //packing settings    
    LRFwindow::saveLRFmakeSettingsToJson();
    SaveJsonToFile(SensLRF->LRFmakeJson, "makeLRF.json");

    MW->WindowNavigator->ResetAllProgressBars();
    MW->WindowNavigator->BusyOn();
    ui->pbStopLRFmake->setEnabled(true);
    qApp->processEvents();

    bool ok = SensLRF->onlyGains(SensLRF->LRFmakeJson, EventsDataHub, PMs);

    if (!ok) message(SensLRF->getLastError(), this);
    LRFwindow::on_pbUpdateHistory_clicked(); //updating iteration history indication
    ui->pbStopLRFmake->setEnabled(false);
    ui->pbStopLRFmake->setText("Stop");
    MW->WindowNavigator->BusyOff(false);
}

void LRFwindow::onProgressReportReceived(int progress)
{
  MW->WindowNavigator->setProgress(progress);
  ui->pbarLRF->setValue(progress);
}

void LRFwindow::DoMakeGroupLRF(int igroup)
{
  qDebug()<< "\nScript requested to make group LRF, group #:"<<igroup;
  if (igroup<0 || igroup>= SensLRF->countGroups())
    {
      qDebug() << "Wrong iteration group selected!";
      return;
    }
  ui->cbMakePMGroup->setChecked(true);
  ui->sbMakePMgroup->setValue(igroup);
  LRFwindow::on_pbMakeLRF_clicked();
  ui->cbMakePMGroup->setChecked(false);
}

void LRFwindow::drawRadial()
{
   ALrfDraw lrfd(MW->Detector->LRFs, true, EventsDataHub, PMs, MW->GraphWindow);

   QJsonObject json;
   json["DataSource"] =  ui->cb_data_selector->currentIndex(); //0 - sim, 1 - loaded
   json["PlotLRF"] = ui->cb_LRF_2->isChecked();
   json["PlotSecondaryLRF"] = ( (secondIter!=-1) && (secondIter < SensLRF->countIterations()) );
   json["PlotData"] = ui->cb_data_2->isChecked();
   json["PlotDiff"] = ui->cb_diff_2->isChecked();
   json["FixedMin"] = ui->cbFixedMin->isChecked();
   json["FixedMax"] = ui->cbFixedMax->isChecked();
   json["Min"] = ui->ledFixedMin->text().toDouble();
   json["Max"] = ui->ledFixedMax->text().toDouble();
   json["CurrentGroup"] = MW->Detector->PMgroups->getCurrentGroup();
   json["CheckZ"] = ui->cb3D_LRFs->isChecked();
   json["Z"] = ui->ledZcenter->text().toDouble();
   json["DZ"] = ui->ledZrange->text().toDouble();
   json["EnergyScaling"] = ui->cbEnergyScalling->isChecked();
   json["FunctionPointsX"] = MW->GlobSet.FunctionPointsX;
   json["Bins"] = ui->sbNumBins_2->value();
   json["ShowNodes"] = ui->cbShowNodePositions->isChecked();

   QJsonObject js;
   js["SecondIteration"] = secondIter;
   json["ModuleSpecific"] = js;

   lrfd.DrawRadial(ui->sbPMnoButons->value(), json);
}

void LRFwindow::on_pbShowRadialForXY_clicked()
{
    if (!SensLRF->isAllLRFsDefined())
      {
        message("LRFs are not defined!", this);
        return;
      }

    int ipm = ui->sbPMnoButons->value();
    if (ipm > PMs->count()-1)  //protection
      {
        message("Invalid PM number!", this);
        return;
      }
    double z0 = ui->ledZcenter->text().toDouble();
    int nProfiles = ui->sbProfilesForXYtoRad->value();

    QVector<double> Rad, LRF;

    double rr[3];
    rr[2] = z0;
    double rStep = ui->ledStepForXYtoRad->text().toDouble();
    for (int iPr=0; iPr<nProfiles; iPr++)
    {
        double angle = iPr*2*3.1415926535/nProfiles;
        double lrf = 0;
        double radius = 0;
        do
          {
            rr[0] = radius*cos(angle);
            rr[1] = radius*sin(angle);
            lrf = SensLRF->getLRFlocal(ipm, rr);
            if (lrf != 0)
              {                
                Rad << radius;
                LRF << lrf;
              }
            radius += rStep;
          }
        while (lrf != 0);
    }

    MW->GraphWindow->ShowAndFocus();
    TString str = "LRF of pm#";
    str += ipm;
    MW->GraphWindow->MakeGraph(&Rad, &LRF, 4, "Radial distance, mm", "LRF", 6, 1, 0, 0, "");
}

void LRFwindow::on_pbAxial3DvsZ_clicked()
{
    if (!SensLRF->isAllLRFsDefined())
    {
        message("LRFs are not defined!", this);
        return;
    }

    int ipm = ui->sbPMnoButons->value();
    if (ipm > PMs->count()-1)  //protection
    {
        message("Invalid PM number!", this);
        return;
    }

    const LRFaxial3d* lrf = dynamic_cast<const LRFaxial3d*>( (*SensLRF)[ipm] );
    if (!lrf)
    {
        message("This operation is only allowed for Axial3D LRFs", this);
        return;
    }

    double radius = ui->ledAxial3DvsZ->text().toDouble();

    QVector<double> Z, L;
    double z0 = lrf->getZmin();
    double z1 = lrf->getZmax();
    double rr[3];
    rr[0] = radius;
    rr[1] = 0;
    int bins = MW->GlobSet.FunctionPointsX;
    double stepZ = (z1-z0)/bins;
    for (int i=0; i<bins; i++)
    {
        rr[2] = z0 + stepZ*i;
        Z << rr[2];
        L << SensLRF->getLRFlocal(ipm, rr);
    }

    MW->GraphWindow->ShowAndFocus();
    TString str = "LRF of pm#";
    str += ipm;
    str += " at R=";
    str += radius;
    str += " mm";
    TGraph* gr = MW->GraphWindow->ConstructTGraph(Z, L, str, "Z, mm", "LRF", 2,0,0, 2,1,2);
    MW->GraphWindow->Draw(gr, "APL");
}

void LRFwindow::on_pbAxial3DvsRandZ_clicked()
{
    if (!SensLRF->isAllLRFsDefined())
    {
        message("LRFs are not defined!", this);
        return;
    }

    int ipm = ui->sbPMnoButons->value();
    if (ipm > PMs->count()-1)  //protection
    {
        message("Invalid PM number!", this);
        return;
    }

    const LRFaxial3d* lrf = dynamic_cast<const LRFaxial3d*>( (*SensLRF)[ipm] );
    if (!lrf)
    {
        message("This operation is only allowed for Axial3D LRFs", this);
        return;
    }

    QVector<double> R, Z, L;
    double r0 = lrf->getXmin();
    double r1 = lrf->getXmax();
    double z0 = lrf->getZmin();
    double z1 = lrf->getZmax();

    double rr[3];
    rr[1] = 0;
    int binsR = MW->GlobSet.FunctionPointsX;
    int binsZ = MW->GlobSet.FunctionPointsY;
    double stepR = (r1-r0)/binsR;
    double stepZ = (z1-z0)/binsZ;
    for (int ir=0; ir<binsR; ir++)
        for (int iz=0; iz<binsZ; iz++)
        {
            rr[0] = r0 + stepR*ir;
            rr[2] = z0 + stepZ*iz;
            R << rr[0];
            Z << rr[2];
            L << SensLRF->getLRFlocal(ipm, rr);
        }

    MW->GraphWindow->ShowAndFocus();
    TString str = "LRF of pm#";
    str += ipm;
    TGraph2D* gr = MW->GraphWindow->ConstructTGraph2D(R, Z, L, str, "R, mm", "Z, mm", "LRF", 2,0,0, 2,1,2);
    MW->GraphWindow->Draw(gr, "surf");
}

void LRFwindow::on_pbShowErrorVsRadius_clicked()
{
  if (SensLRF->getIteration() == 0) return;
  MW->GraphWindow->ShowAndFocus();

  int PMnumber = ui->sbPMnoButons->value();
  double z0 = ui->ledZcenter->text().toDouble();

  const LRF2 *lrf = (*SensLRF)[PMnumber];
  TF1 *f1d = new TF1("f1derror", SensLRF, &SensorLRFs::getErrorRadial, 0, lrf->getRmax()*1.01, 4,
                "SensorLRFs", "getErrorRadial");
  f1d->SetParameter(0, PMnumber);
  f1d->SetParameter(1, PMs->X(PMnumber));
  f1d->SetParameter(2, PMs->Y(PMnumber));
  f1d->SetParameter(3, z0);

  f1d->GetXaxis()->SetTitle("Radial distance, mm");
  f1d->GetYaxis()->SetTitle("Error");
  TString str = "Error of pm#";
  str += PMnumber;
  f1d->SetTitle(str);
  MW->GraphWindow->DrawWithoutFocus(f1d);

  MW->GraphWindow->LastDistributionShown = "errorVsRadius";
}

void LRFwindow::on_pbUpdateHistory_clicked()
{
  ui->lwLRFhistory->clear();

  if (!SensLRF->isCurrentIterDefined())
    {
//      qDebug()<< "Current iter is NULL";
      ui->leCurrentLRFs->setText("--");
      emit SensLRF->SensorLRFsReadySignal(false);
      return;
    }

  ui->leCurrentLRFs->setText(SensLRF->getIterationName());

  int curIt = SensLRF->getCurrentIterIndex();
//  qDebug() << "--cur it index:" << curIt;
  if (secondIter == curIt) secondIter = -1; //protection

  //updating the list widget
  int numIters = SensLRF->countIterations();
//  qDebug() << "--it groups:"<<numIters;

  for (int iter = numIters-1; iter>-1; iter--)
    {
      QString name = SensLRF->getIterationName(iter);
//      qDebug() << "---- iter:"<<iter<<"name:"<<name;
      QListWidgetItem* pItem =new QListWidgetItem(name);
      if (iter == secondIter)
        {          
          pItem->setText(name);
          pItem->setBackgroundColor(QColor(55,142,192));
          pItem->setForeground(Qt::white);
        }
      else if (iter == curIt)
        {
          //name = name;
          pItem->setText(name);
          pItem->setForeground(Qt::red);
          QFont font = pItem->font();
          font.setBold(true);
          pItem->setFont(font);
        }
      else pItem->setForeground(Qt::black);


      QString tt = name;
    //  tt += "<br>---------------------------";
      tt += "<br>"+SensLRF->getIteration(iter)->getGrouppingOption();
      const LRF2* lrf = SensLRF->getIteration(iter)->getGroups()->first().getLRF();
      if (lrf)
        {
          tt += "<br>"+QString(lrf->type());
          QJsonObject json = lrf->reportSettings();
          if (json.contains("n1"))
            {
              tt += "<br>"+ QString::number(json["n1"].toInt());
              if (json.contains("n2")) tt += " x "+ QString::number(json["n2"].toInt());
            }
        }
      pItem->setToolTip(tt);

      ui->lwLRFhistory->addItem(pItem);
    }
}

void LRFwindow::on_leCurrentLRFs_editingFinished()
{
  SensLRF->setCurrentIterName(ui->leCurrentLRFs->text());
  LRFwindow::on_pbUpdateHistory_clicked();
  LRFwindow::on_pbUpdateLRFmakeJson_clicked();
}

void LRFwindow::historyClickedTimeout()
{
  //was a single click
  fOneClick = false;

  int row = ui->lwLRFhistory->currentRow();
//  qDebug() << "Single-clicked row:" << row;
  int numIters = SensLRF->countIterations();
  int thisSel = numIters-1 - row;
  if (thisSel == secondIter) secondIter = -1;
  else secondIter = thisSel;

  LRFwindow::on_pbUpdateHistory_clicked();
}

void LRFwindow::on_lwLRFhistory_itemClicked(QListWidgetItem * /*item*/)
{
  if (!fOneClick)
    {
      //giving user chance to click again - detecting double click
      timer.singleShot(300, this, SLOT(historyClickedTimeout()));
      fOneClick = true;
      return;
    }

  //already the second click!
  timer.stop();
  int row = ui->lwLRFhistory->currentRow();
//  qDebug() << "Double clicked row:" << row;
  int numIters = SensLRF->countIterations();
  int iterSelected = numIters-1 - row;
  SensLRF->setCurrentIter(iterSelected);
  if (secondIter == iterSelected) secondIter = -1;

  LRFwindow::updateIterationIndication();
  on_pbUpdateLRFmakeJson_clicked();
}

void LRFwindow::updateIterationIndication()
{
    IterationGroups* iter = SensLRF->getIteration();
    if (iter)
      {
        //qDebug() << iter->makeJson;
        if (!iter->makeJson.isEmpty())
          {
            loadJSON(iter->makeJson);  //new system
          }
        else
        {
            // old system - info is missing so using default settings
            ui->cb_data_selector->setCurrentIndex(0);
            ui->cb_use_grid->setChecked(true);
            ui->cbForceDerivToZeroInOrigin->setChecked(true);
            ui->cb_store_error->setChecked(false);
            ui->cbEnergyScalling->setChecked(true);
            ui->cbUseGroupping->setChecked(false);
            ui->cb_adj_gains->setChecked(true);
            //update history widget
            LRFwindow::on_pbUpdateHistory_clicked();
            LRFwindow::on_pbUpdateGUI_clicked();

            QJsonObject json = (*SensLRF)[0]->reportSettings();
            LRFwindow::updateLRFinfo(json);
            //update groupping option
            LRFwindow::updateGroupsInfo();
        }
      }
    on_pbUpdateLRFmakeJson_clicked(); //to Config->JSON
}

void LRFwindow::updateGroupsInfo()
{
  tmpIgnore = true;
  IterationGroups *iteration = SensLRF->getIteration();
  if(!iteration)
  {
      qWarning()<<"LRFwindow::updateGroupsInfo() currentIteration is NULL";
      return;
  }
  QString option = iteration->getGrouppingOption();
  //qDebug() << "Groupping option: " << option;
  if (option == "Individual")
    {
      ui->cbUseGroupping->setChecked(false);
      ui->fPMgroups->setEnabled(false);
    }
  else
    {
      ui->cbUseGroupping->setChecked(true);
      ui->fPMgroups->setEnabled(true);
      if (option.startsWith("One LRF")) ui->cb_pmt_group_type->setCurrentIndex(0);
      else if (option.startsWith("Group PMs by radius")) ui->cb_pmt_group_type->setCurrentIndex(1);
      else if (option.startsWith("Assume square")) ui->cb_pmt_group_type->setCurrentIndex(2);
      else if (option.startsWith("Assume hexagonal")) ui->cb_pmt_group_type->setCurrentIndex(3);
      else
        {
          ui->cbUseGroupping->setChecked(false);
          ui->fPMgroups->setEnabled(false);
          //message("Unknown groupping option!", this);
        }
    }

  tmpIgnore = false;
}


void LRFwindow::updateLRFinfo(QJsonObject &json)
{
  //qDebug() << json;
  tmpIgnore = true; //starting bulk update
  JsonParser parser(json);

  //LRF type
  QString type;
  if (!parser.ParseObject("type", type))
    {
      message("Unknown format!", this);
      return;
    }

  //reconstruct Z?
  if (type == "Axial3D" || type == "Radial3D" || type == "Sliced3D")
    {
      ui->cb3D_LRFs->setChecked(true);
      ui->swLRFtype->setCurrentIndex(1);
    }
  else
    {
      ui->cb3D_LRFs->setChecked(false);
      ui->swLRFtype->setCurrentIndex(0);
    }

  //type indicator
  if (type == "Axial" || type == "Radial" || type == "ComprAxial" || type == "ComprRad" ) ui->cob2Dtype->setCurrentIndex(0);
  if (type == "XY" || type == "Freeform") ui->cob2Dtype->setCurrentIndex(1);
  if (type == "Polar") ui->cob2Dtype->setCurrentIndex(2);
  if (type == "Composite") ui->cob2Dtype->setCurrentIndex(3);
  if (type == "Axial3D" || type == "Radial3D") ui->cob3Dtype->setCurrentIndex(0);
  if (type == "Sliced3D") ui->cob3Dtype->setCurrentIndex(1);

  //compress panel?
  if (type == "Axial" || type == "Radial" || type == "ComprAxial" || type == "ComprRadial" || type == "Composite")
    {
      ui->fCompression->setVisible(true);

      if (type == "ComprAxial" || type == "ComprRad")
        {
          ui->cb_compress_r->setChecked(true);
          double tmp;
          parser.ParseObject("r0", tmp);
          ui->led_compression_r0->setText(QString::number(tmp));
          parser.ParseObject("comp_lam", tmp);
          ui->led_compression_lam->setText(QString::number(tmp));
          parser.ParseObject("comp_k", tmp);
          ui->led_compression_k->setText(QString::number(tmp));
        }
      else if (type == "Composite")
        {
          //expecting first LRF in th deck to be Axial or cAxial
          QVector<QJsonObject> LRFdeck;
          QJsonArray deck;
           ui->cb_compress_r->setChecked(false);
          if (parser.ParseObject("deck", deck))
           if (parser.ParseArray(deck, LRFdeck))
            if (LRFdeck.size()>0)
              {
                QJsonObject axLRF = LRFdeck[0];
                QString type = axLRF["type"].toString();
                /////if (1) // (LRFdeck[0]["type"] == "ComprAxial" || LRFdeck[0]["type"] == "ComprRad")
                if (type == "ComprAxial" || type == "ComprRad")
                {
  //                qDebug() << "-----" << LRFdeck[0]["type"];
                  JsonParser parser1(LRFdeck[0]);
                  ui->cb_compress_r->setChecked(true);
                  double tmp;
                  parser1.ParseObject("r0", tmp);
                  ui->led_compression_r0->setText(QString::number(tmp));
                  parser1.ParseObject("comp_lam", tmp);
                  ui->led_compression_lam->setText(QString::number(tmp));
                  parser1.ParseObject("comp_k", tmp);
                  ui->led_compression_k->setText(QString::number(tmp));
                }
              }

        }
      else ui->cb_compress_r->setChecked(false);
    }
  else ui->fCompression->setVisible(false);

  //number of nodes
  int val;
  if (parser.ParseObject("n1", val)) ui->sbSplNodesX->setValue(val);
  if (parser.ParseObject("n2", val)) ui->sbSplNodesY->setValue(val);

  tmpIgnore = false;
  LRFwindow::on_pbUpdateGUI_clicked(); //additional interface update
}

void LRFwindow::on_pushButton_clicked()
{
    double wx = ui->ledWx->text().toDouble();
    double wy = ui->ledWy->text().toDouble();

    double rw[3] = {wx, wy, 0};
    double rl[3];
    SensLRF->getSensor(ui->spinBox->value())->transform(rw, rl);

    ui->ledLx->setText(QString::number(rl[0]));
    ui->ledLy->setText(QString::number(rl[1]));
}

void LRFwindow::on_pushButton_2_clicked()
{
  double lx = ui->ledLx->text().toDouble();
  double ly = ui->ledLy->text().toDouble();

  double rl[3] = {lx, ly, 0};
  double rw[3];
  SensLRF->getSensor(ui->spinBox->value())->transformBack(rl, rw);

  ui->ledWx->setText(QString::number(rw[0]));
  ui->ledWy->setText(QString::number(rw[1]));
}

void LRFwindow::contextMenuHistory(const QPoint &pos)
{
  QMenu historyMenu;
  int row;

  QListWidgetItem* temp = ui->lwLRFhistory->itemAt(pos);
  if(temp != NULL) row = ui->lwLRFhistory->row(temp);
  else return;

  historyMenu.addSeparator();
  QAction* cur = historyMenu.addAction("Select as current");
  historyMenu.addSeparator();
  QAction* sec = historyMenu.addAction("Select as second");
  historyMenu.addSeparator();
  QAction* del =historyMenu.addAction("Delete");
  historyMenu.addSeparator();

  QAction* selectedItem = historyMenu.exec(ui->lwLRFhistory->mapToGlobal(pos));

  int numIters = SensLRF->countIterations();
  int iter = numIters-1 - row;

  if (selectedItem == cur)
    {
      //Setting as current
      SensLRF->setCurrentIter(iter);
      if (secondIter == iter) secondIter = -1;
      updateIterationIndication();
    }
  else if (selectedItem == sec)
    {
      //Setting as second
      if (iter == secondIter) secondIter = -1;
      else secondIter = iter;
    }
  else if (selectedItem == del)
    {
      //Delete iter
      if (iter == SensLRF->getCurrentIterIndex())
        {
          //deleting actual current
          QMessageBox::StandardButton reply;
          reply = QMessageBox::question(this, "Confirmation needed", "Delete currently selected LRFs?",
                                          QMessageBox::Yes|QMessageBox::Cancel);
          if (reply == QMessageBox::Cancel) return;
        }

      SensLRF->deleteIteration(iter); //it takes care of current/not current and being last iter
      secondIter = -1;

      LRFwindow::updateIterationIndication();
    }

  LRFwindow::on_pbUpdateHistory_clicked();
  LRFwindow::on_pbUpdateLRFmakeJson_clicked();
}

void LRFwindow::on_pbClearHistory_clicked()
{
  QMessageBox::StandardButton reply;
  reply = QMessageBox::question(this, "Confirmation needed", "Delete all history?",
                                  QMessageBox::Yes|QMessageBox::Cancel);
  if (reply == QMessageBox::Cancel) return;
  SensLRF->clearHistory();
  LRFwindow::on_pbUpdateHistory_clicked();
}

void LRFwindow::on_pbClearTmpHistory_clicked()
{
  QMessageBox::StandardButton reply;
  reply = QMessageBox::question(this, "Confirmation needed", "Delete all iterations with --#-- name pattern?",
                                  QMessageBox::Yes|QMessageBox::Cancel);
  if (reply == QMessageBox::Cancel) return;

  SensLRF->clearHistory(true);
  LRFwindow::on_pbUpdateHistory_clicked();
}

void LRFwindow::on_pbExpand_clicked()
{
   ui->swAdvancedFeatures->setCurrentIndex(1);
   this->setFixedSize(690, this->height());
}

void LRFwindow::on_pbShrink_clicked()
{
   ui->swAdvancedFeatures->setCurrentIndex(0);
   this->setFixedSize(554, this->height());
}

void LRFwindow::on_pbRadialToText_clicked()
{
    //watchdogs
    if (!SensLRF->isAllLRFsDefined())
      {
        message("There is no LRF data!", this);
        return;
      }
    double energy = 1.0;
    if (ui->cbScaleForEnergy->isChecked())
    {
        //if (EventsDataHub->ReconstructionData.isEmpty() || !MW->Reconstructor->isReconstructionDataReady() )
        if (!EventsDataHub->isReconstructionReady() )
          {
            message("Reconstruction was not yet performed, energy scaling is not possible!", this);
            return;
          }

        int goodEv = 0;
        energy = 0;
        for (int iev=0; iev<EventsDataHub->ReconstructionData[0].size(); iev++)
        {
            if (EventsDataHub->ReconstructionData[0][iev]->GoodEvent)
            {
                goodEv++;
                energy += EventsDataHub->ReconstructionData[0][iev]->Points[0].energy;
            }
        }
        if (goodEv == 0)
        {
            message("There are no events passing all the filters!", this);
            return;
        }
        energy /= goodEv;
        //qDebug() << "average energy: "<< energy;
    }

    QString str = ui->sbPMnoButons->text();
    QString fileName = QFileDialog::getSaveFileName(this, "Save LRF # " +str+ " vs radius", MW->GlobSet.LastOpenDir,
                                                   "Text files (*.txt);;Data files (*.dat);;All files (*.*)");
    if (fileName.isEmpty()) return;
    MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

    QFile outputFile(fileName);
    outputFile.open(QIODevice::WriteOnly);
    if(!outputFile.isOpen())
      {
        message("Unable to open file " +fileName+ " for writing!", this);
        return;
      }
    QTextStream outStream(&outputFile);

    double step = ui->ledStep->text().toDouble();
    double p[2];  //p[0] is pm number, p[1] is z
    p[1] = MW->Rwindow->getSuggestedZ();
    double r[1];
    int ipm = ui->sbPMnoButons->value();

    PMsensor* sens = SensLRF->getSensor(ipm);
    const LRF2 *lrf = sens->GetLRF();
    double rmax = lrf->getRmax();
    double gain = 1.0;
    if (ui->cbScaleForGain->isChecked()) gain = sens->GetGain();

    p[0] = ipm;

    for (r[0] = 0.0; r[0]<=rmax; r[0] += step)
    {
        double val = SensLRF->getRadial(r, p)*gain*energy;
        outStream << r[0] << " " << val << "\n";
    }

    outputFile.close();
}

void LRFwindow::on_pbShowSensorGroups_clicked()
{
  QVector<QString> tmp(PMs->count());

  for (int igrp=0; igrp<SensLRF->countGroups(); igrp++)
    {
      const std::vector<PMsensor> *sensors = SensLRF->getSensorGroup(igrp)->getPMs();
      for(unsigned int j = 0; j < sensors->size(); j++)
        tmp[(*sensors)[j].GetIndex()].append(QString::number(igrp,'g', 4));
    }

  MW->clearGeoMarkers();
  MW->GeometryWindow->on_pbTop_clicked();
  MW->GeometryWindow->ShowGeometry();
  MW->GeometryWindow->ShowTextOnPMs(tmp, kBlue);
}

void LRFwindow::on_pbShowSensorGains_clicked()
{
    QVector<QString> tmp;

    for (int ipm=0; ipm<PMs->count(); ipm++)
    {
        double gain = SensLRF->getSensor(ipm)->GetGain();
        tmp.append(QString::number(gain, 'g', 4));
    }

    MW->clearGeoMarkers();
    MW->GeometryWindow->on_pbTop_clicked();
    MW->GeometryWindow->ShowGeometry();
    MW->GeometryWindow->ShowTextOnPMs(tmp, kBlue);
}

void LRFwindow::on_led_compression_k_editingFinished()
{
  double val = ui->led_compression_k->text().toDouble();
  if (val<=1)
    {
      message("Compression factor should be greater than 1.0. Setting to 3.0", this);
      ui->led_compression_k->setText("3.0");
    }
}

void LRFwindow::on_cbUseGroupping_toggled(bool checked)
{
  QString txt;
  if (checked) txt = "Only for group#:";
  else         txt = "Only for PM#:";
  ui->cbMakePMGroup->setText(txt);
  ui->fSingleGroup->setEnabled(checked);
}

void LRFwindow::on_pbTableToAxial_clicked()
{
    ui->cbUseGroupping->setChecked(false);
    ui->cb3D_LRFs->setChecked(false);
    ui->cob2Dtype->setCurrentIndex(0);
    ui->cbMakePMGroup->setChecked(false);

    QString fileName = QFileDialog::getOpenFileName(this, "LRF table file", MW->GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*.*)");
    if (fileName.isEmpty()) return;
    //MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

    QFileInfo fi(fileName);
    QString ext = fi.suffix();
    QString base = fi.baseName();
    int index = 1;
    while (index < base.size())
      {
        bool ok;
        //qDebug() << base.right(index);
        int tmp = base.right(index).toInt(&ok);
        if (!ok || tmp<0) break;
        index++;
      }
    if (index == 1 || index == base.size())
      {
        message("Name pattern should end with PM number", this);
        return;
      }
    QString Pattern = fi.absolutePath() + "/" + base.left(base.length()-index+1) + "." +ext;
    qDebug() << "file pattern to be used:" << Pattern;

    QJsonObject json;
    writeToJson(json);
    SaveJsonToFile(json, "makeLRF.json");

    bool ok = SensLRF->makeAxialLRFsFromRfiles(json, Pattern, PMs);
    if (!ok) message(SensLRF->getLastError(), this);
    LRFwindow::on_pbUpdateHistory_clicked(); //updating iteration history indication
}

void LRFwindow::on_bConfigureFiltering_clicked()
{
    MW->Rwindow->show();
    MW->Rwindow->ShowFiltering();
}

void LRFwindow::on_pbUpdateLRFmakeJson_clicked()
{
      //qDebug() << "LRFmakeGUI->JSON";
    writeToJson(SensLRF->LRFmakeJson);
    MW->Config->UpdateLRFmakeJson();
    if (MW->ScriptWindow)
        if (MW->ScriptWindow->isVisible())
            MW->ScriptWindow->updateJsonTree();
}

void LRFwindow::on_actionPM_numbers_triggered()
{
    MW->GeometryWindow->ShowPMnumbers();
}

void LRFwindow::on_actionPM_groups_triggered()
{
  QVector<QString> tmp(PMs->count());

  for (int igrp=0; igrp<SensLRF->countGroups(); igrp++)
  {
      const std::vector<PMsensor> *sensors = SensLRF->getSensorGroup(igrp)->getPMs();
      for(unsigned int j = 0; j < sensors->size(); j++)
        tmp[(*sensors)[j].GetIndex()].append(QString::number(igrp,'g', 4));
  }
  MW->GeometryWindow->ShowTextOnPMs(tmp, kBlue);
}

void LRFwindow::on_actionRelative_gain_triggered()
{
  QVector<QString> tmp(0);

  for (int i=0; i<PMs->count(); i++)
      tmp.append(QString::number(SensLRF->getSensor(i)->GetGain(),'g', 4));

  MW->GeometryWindow->ShowTextOnPMs(tmp, kBlue);
}

void LRFwindow::on_actionX_shift_triggered()
{
    ShowTransform(0);
}

void LRFwindow::on_actionY_shift_triggered()
{
    ShowTransform(1);
}

void LRFwindow::on_actionRotation_Flip_triggered()
{
    ShowTransform(2);
}

void LRFwindow::ShowTransform(int type)
{
    QVector<QString> tmp(0);
    double t[3];
    bool flip;

    for (int i=0; i<PMs->count(); i++) {
        SensLRF->getSensor(i)->GetTransform(t, t+1, t+2, &flip);
        t[2] = t[2]/3.1415926*180;
        tmp.append(QString::number(t[type%3],'g', 4).append(flip ? "F" : ""));
    }

    MW->GeometryWindow->ShowTextOnPMs(tmp, kBlue);
}

void LRFwindow::on_pbLRFexplorer_clicked()
{
    ALrfMouseExplorer* ME = new ALrfMouseExplorer(MW->Detector->LRFs, PMs, MW->Detector->PMgroups, MW->Rwindow->getSuggestedZ(), this);
    ME->Start();
    ME->deleteLater();
}

void LRFwindow::on_cb_data_clicked()
{
    bool data = ui->cb_data->isChecked();
    bool dif  = ui->cb_diff->isChecked();

    if (data && dif) ui->cb_diff->setChecked(false);
}

void LRFwindow::on_cb_diff_clicked()
{
    bool data = ui->cb_data->isChecked();
    bool dif  = ui->cb_diff->isChecked();

    if (data && dif) ui->cb_data->setChecked(false);
}

void LRFwindow::on_cbEnergyScalling_toggled(bool)
{
    updateWarningForEnergyScan();
}

void LRFwindow::on_cb_data_selector_currentIndexChanged(int)
{
    updateWarningForEnergyScan();
}

void LRFwindow::updateWarningForEnergyScan()
{
  ui->pbWarningEnergyWithScan->setVisible( ui->cbEnergyScalling->isChecked() && ui->cb_data_selector->currentIndex()==0 );
}
