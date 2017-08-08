//ANTS2
#include "reconstructionwindow.h"
#include "ui_reconstructionwindow.h"
#include "mainwindow.h"
#include "detectorclass.h"
#include "geometrywindowclass.h"
#include "graphwindowclass.h"
#include "pms.h"
#include "pmtypeclass.h"
#include "outputwindow.h"
#include "sensorlrfs.h"
#include "alrfmoduleselector.h"
#include "lrfwindow.h"
#include "CorrelationFilters.h"
#include "windownavigatorclass.h"
#include "guiutils.h"
#include "ajsontools.h"
#include "gainevaluatorwindowclass.h"
#include "viewer2darrayobject.h"
#include "genericscriptwindowclass.h"
#include "eventsdataclass.h"
#include "dynamicpassiveshandler.h"
#include "reconstructionmanagerclass.h"
#include "globalsettingsclass.h"
#include "arootlineconfigurator.h"
#include "ageomarkerclass.h"
#include "apositionenergyrecords.h"
#include "aenergydepositioncell.h"
#include "amessage.h"
#include "afiletools.h"
#include "acommonfunctions.h"
#include "aconfiguration.h"
#include "apmgroupsmanager.h"
#include "asandwich.h"
#include "alrfwindow.h" //For new LRF module
#include "arepository.h"
#include "amaterialparticlecolection.h"
#include "tmpobjhubclass.h"

#ifdef __USE_ANTS_CUDA__
#include "cudamanagerclass.h"
#endif

#ifdef ANTS_FANN
#include "neuralnetworksmodule.h"
#include "neuralnetworkswindow.h"
#endif

//Qt
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QDialog>
#include <QTextEdit>
#include <QDesktopServices>
#include <QDebug>
#include <QMenu>
#include <QInputDialog>

//Root
#include "TMath.h"
#include "TGeoManager.h"
#include "TVirtualGeoTrack.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TF2.h"
#include "TFile.h"
#include "TLine.h"
#include "TEllipse.h"
#include "TBox.h"
#include "TSystem.h"
#include "TPolyLine.h"
#include "TMultiGraph.h"
#include "TColor.h"
#include "TFitResultPtr.h"
#include "TFitResult.h"
#include "TRandom2.h"

ReconstructionWindow::~ReconstructionWindow()
{
  if (vo) delete vo;
  vo = 0;
  if (dialog)
    {
      disconnect(vo, SIGNAL(PMselectionChanged(QVector<int>)), this, SLOT(onSelectionChange(QVector<int>)));
      delete dialog;
      dialog = 0;
    }

  delete ui;
  if (CorrCutLine) delete CorrCutLine;
  if (CorrCutEllipse) delete CorrCutEllipse;
  if (CorrCutPolygon) delete CorrCutPolygon;

  for (int i=0; i<CorrelationFilters.size(); i++)
      delete CorrelationFilters[i];
  CorrelationFilters.clear();
  delete tmpCorrFilter;

  if (MW->GainWindow)
    {
      //qDebug()<<"  -<Deleting gain evaluation window";
      MW->GainWindow->hide();
      delete MW->GainWindow->parent();     
      MW->GainWindow = 0;
      //qDebug() << "  --<Deleted";
    }
  //qDebug() << " -<Destructor of Reconstruction window finished";
}

void ReconstructionWindow::writeToJson(QJsonObject &json) //fVerbose - saving as config including LRFs, gui info and info on all algorithms
{
  ReconstructionWindow::updateReconSettings();
  ReconstructionWindow::updateFilterSettings();

  QJsonObject js = MW->Config->JSON["ReconstructionConfig"].toObject();
  int versionNumber = ANTS2_VERSION;
  js["ANTS2build"] = versionNumber;

  //ReconstructionWindow::writePMrelatedInfoToJson(js);
  MW->Detector->PMgroups->writeSensorGroupsToJson(js);

  QJsonObject jsLRF;
  MW->lrfwindow->writeToJson(jsLRF);  //LRF make settings
  js["LRFmakeJson"] = jsLRF;

  //QJsonObject js1;
  //js1["ReconstructionConfig"] = js;
  //SaveJsonToFile(js, "ReconstructionConfig.json");

  writeLrfModuleSelectionToJson(js);

  //old module LRFs
  if (Detector->LRFs->isAllLRFsDefined(true))
    {
      QJsonObject LRFjson;
      Detector->LRFs->saveActiveLRFs_v2(LRFjson);
      js["ActiveLRF"] = LRFjson;
    }
  else
    if (js.contains("ActiveLRF")) js.remove("ActiveLRF");
  //new module LRFs
  if (Detector->LRFs->isAllLRFsDefined(false))
    {
      QJsonObject LRFjson;
      Detector->LRFs->saveActiveLRFs_v3(LRFjson);
      js["ActiveLRFv3"] = LRFjson;
    }
  else
    if (js.contains("ActiveLRFv3")) js.remove("ActiveLRFv3");

  json["ReconstructionConfig"] = js;
}

void ReconstructionWindow::onRequestReconstructionGuiUpdate()
{
    //qDebug() << "RecJson->RecGUI";
    ReconstructionWindow::readFromJson(MW->Config->JSON);
}

bool ReconstructionWindow::readFromJson(QJsonObject &json)
{
  if (!json.contains("ReconstructionConfig"))
    {
      qDebug()<<"||| Json does not contain reconstruction settings";
      return false;
    }
  QJsonObject js = json["ReconstructionConfig"].toObject();
  //sensor groups
  MW->Detector->PMgroups->readGroupsFromJson(js);
  onUpdatePMgroupIndication();  
  //gen recons settings (algorithm)
  readReconSettingsFromJson(js);
  //event filter settings
  readFilterSettingsFromJson(js);
  //LRF module selection
  readLrfModuleSelectionFromJson(js);
  //scripts, PlotXY
  if (js.contains("GuiMisc")) readMiscGUIsettingsFromJson(js);  
  //LRF make settings  
  if (js.contains("LRFmakeJson"))
  {
      QJsonObject jsLRF = js["LRFmakeJson"].toObject();
      MW->lrfwindow->loadJSON(jsLRF);
  }
  else MW->lrfwindow->updateIterationIndication();  //compatibility

#ifdef ANTS_FANN
  if (js.contains("FANNsettings"))
    {
      QJsonObject fannJson = js["FANNsettings"].toObject();
      MW->NNwindow->loadJSON(fannJson);
    }
#endif

  onUpdateGainsIndication();
  onUpdatePassiveIndication();
  RefreshNumEventsIndication();
  ShowStatistics();

  return true;
}

void ReconstructionWindow::UpdateStatusAllEvents()
{    
  //qDebug() << "UpdateStatus all events triggered";
  UpdateReconConfig(); //Config->JSON update

  if (EventsDataHub->isEmpty()) return;

  ReconstructionManager->filterEvents(MW->Config->JSON, MW->GlobSet->NumThreads);
  //   qDebug() << "  Found good events:"<<EventsDataHub->countGoodEvents();
  //qDebug() << "Update done";
}

void ReconstructionWindow::InitWindow()
{  
  ReconstructionWindow::on_pbUpdateFilters_clicked();
  ReconstructionWindow::on_pbUpdateGainsIndication_clicked();
  ReconstructionWindow::on_pbCorrUpdateTMP_clicked(); //update correlation filter designer
  tmpCorrFilter->Active = true;
  CorrelationFilterStructure* NewFilter = new CorrelationFilterStructure(tmpCorrFilter);
  NewFilter->Active = true;
  CorrelationFilters.append(NewFilter);  
}

void ReconstructionWindow::on_cobReconstructionAlgorithm_currentIndexChanged(int index)
{
    if (index==6) index = 2; //same tab for both
    ui->swReconstructionAlgorithm->setCurrentIndex(index);
    ReconstructionWindow::updateRedStatusOfRecOptions(); //update warning indicator in tabWidget
    if (index == 1 || index == 2 || index == 4 ) ui->fDynPassive->setVisible(true);
    else ui->fDynPassive->setVisible(false);  
    ui->cobMultipleOption->setEnabled(index == 2);
}

void ReconstructionWindow::on_sbEventNumberInspect_valueChanged(int arg1)
{
   if (ForbidUpdate) return;
   if (EventsDataHub->isEmpty()) return;

   int totNumEvents = EventsDataHub->Events.size();
   if (totNumEvents == 0) return;
   if (arg1 > totNumEvents-1)
     {
       ui->sbEventNumberInspect->setValue(totNumEvents-1);
       return;
     }

   if (MW->GraphWindow->isVisible())
     {
       if (MW->GraphWindow->LastDistributionShown == "ShowExpectedVsMeasured")
         if (Detector->LRFs->isAllLRFsDefined()) ReconstructionWindow::on_pbShowMeasuredVsExpected_clicked();
       if (MW->GraphWindow->LastDistributionShown == "MapEvent") ReconstructionWindow::on_pbShowMap_clicked();
     }

  int CurrentGroup = PMgroups->getCurrentGroup();
  AReconRecord* result = EventsDataHub->ReconstructionData[CurrentGroup][arg1];
  MW->Owindow->SetCurrentEvent(ui->sbEventNumberInspect->value());

  if (MW->GeometryWindow->isVisible())
    {
      MW->clearGeoMarkers();
      MW->Detector->GeoManager->ClearTracks();

      if (!EventsDataHub->isScanEmpty()) ReconstructionWindow::UpdateSimVizData(arg1);
      if (result->ReconstructionOK)
        {
          GeoMarkerClass* marks = new GeoMarkerClass("Recon", 6, 2, kRed);
          for (int i=0; i<result->Points.size(); i++)
            marks->SetNextPoint(result->Points[i].r[0], result->Points[i].r[1], result->Points[i].r[2]);         
          MW->GeoMarkers.append(marks);
        }
      MW->ShowGeometry(false);  //to clear view
      MW->ShowTracks(); //has to use ShowTracks since if there is continuos energy deposition - tracks are used for inidication
    }
}

void ReconstructionWindow::UpdateSimVizData(int iev)
{
  if (EventsDataHub->isScanEmpty()) VisualizeEnergyVector(iev);
  else VisualizeScan(iev);
}

void ReconstructionWindow::VisualizeScan(int iev)
{
  GeoMarkerClass* marks;
  bool fAppend;
  if (MW->GeoMarkers.isEmpty() || MW->GeoMarkers.last()->Type != "Scan")
    { //previous marker was not scan, so making new one
      marks = new GeoMarkerClass("Scan", 6, 2, kBlue);
      fAppend = true;
    }
  else
    { //reusing old scan marker, just add the new point. This way its is MUCH faster to remove markers if one has ~100k o them.
      marks = MW->GeoMarkers.last();
      fAppend = false;
    }

  if ( EventsDataHub->Scan[iev]->ScintType == 2)  //secondary scint
    {
      for (int j=0; j<EventsDataHub->Scan[iev]->Points.size(); j++)
        {
          marks->SetNextPoint(EventsDataHub->Scan[iev]->Points[j].r[0], EventsDataHub->Scan[iev]->Points[j].r[1], EventsDataHub->Scan[iev]->Points[j].r[2]);
          marks->SetNextPoint(EventsDataHub->Scan[iev]->Points[j].r[0], EventsDataHub->Scan[iev]->Points[j].r[1], EventsDataHub->Scan[iev]->zStop);
          //showing track
          Int_t track_index = MW->Detector->GeoManager->AddTrack(10,55); //  Here track_index is the index of the newly created track in the array of primaries. One can get the pointer of this track and make it known as current track by the manager class:
          TVirtualGeoTrack *track = MW->Detector->GeoManager->GetTrack(track_index);
          track->SetLineColor(kBlue);
          track->AddPoint( EventsDataHub->Scan[iev]->Points[j].r[0], EventsDataHub->Scan[iev]->Points[j].r[1], EventsDataHub->Scan[iev]->Points[j].r[2], 0);
          track->AddPoint( EventsDataHub->Scan[iev]->Points[j].r[0], EventsDataHub->Scan[iev]->Points[j].r[1], EventsDataHub->Scan[iev]->zStop, 0);
        }
    }
  else
    {
      for (int j=0; j<EventsDataHub->Scan[iev]->Points.size(); j++)
        marks->SetNextPoint(EventsDataHub->Scan[iev]->Points[j].r[0], EventsDataHub->Scan[iev]->Points[j].r[1], EventsDataHub->Scan[iev]->Points[j].r[2]);
    }  

  if (fAppend) MW->GeoMarkers.append(marks);
}

void ReconstructionWindow::on_pbClearPositions_clicked()
{
    MW->clearGeoMarkers();
    MW->ShowGeometry(false);
}

void ReconstructionWindow::on_pbShowReconstructionPositions_clicked()
{    
    ShowPositions(0);
}

void ReconstructionWindow::on_pbShowTruePositions_clicked()
{
    ShowPositions(1);
}

void ReconstructionWindow::ShowPositions(int Rec_True, bool fOnlyIfWindowVisible)
{
    if (fOnlyIfWindowVisible && !MW->GeometryWindow->isVisible()) return;
    if (Rec_True<0 || Rec_True>1)
    {
        qWarning() << "Wrong index in ShowPositions";
        return;
    }

    int CurrentGroup = PMgroups->getCurrentGroup();
    bool fRecReady = EventsDataHub->isReconstructionReady(CurrentGroup);

    int numEvents;
    GeoMarkerClass* marks;
    if (Rec_True == 0)
    {
        int CurrentGroup = PMgroups->getCurrentGroup();
        if (!fRecReady)
        {
            message("Reconstruction not ready!", this);
            return;
        }
        numEvents = EventsDataHub->ReconstructionData[CurrentGroup].size();
        marks = new GeoMarkerClass("Recon", 6, 2, kRed);
    }
    else if (Rec_True == 1)
    {
        if (EventsDataHub->isScanEmpty())
        {
            message("There are no true positions available!", this);
            return;
        }
        numEvents = EventsDataHub->Scan.size();
        marks = new GeoMarkerClass("Scan", 6, 2, kBlue);
    }

    MW->Detector->GeoManager->ClearTracks(); //tracks could be used for indication (if continuous deposition)
    MW->clearGeoMarkers(Rec_True+1);

    int goodCounter = 0;
    int everyPeriod = ui->sbShowEventsEvery->value();
    int everyCounter = 0;

    int UpperLimit = numEvents;
    if (ui->cbLimitNumberEvents->isChecked()) UpperLimit = ui->sbLimitNumberEvents->value();

    for (int iev=0; iev<numEvents; iev++)
      {
        if (fRecReady && !EventsDataHub->ReconstructionData.at(CurrentGroup).at(iev)->GoodEvent) continue;

        everyCounter++;
        if (everyCounter == everyPeriod)
            {
                if (Rec_True == 0)
                {
                    for (int iP = 0; iP<EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points.size(); iP++)
                    marks->SetNextPoint(EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[iP].r[0],
                                        EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[iP].r[1],
                                        EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[iP].r[2]);
                }
                else
                {
                    ReconstructionWindow::VisualizeScan(iev);
                    if ( EventsDataHub->Scan[iev]->ScintType == 2)  //secondary scint
                      {
                        for (int j=0; j<EventsDataHub->Scan[iev]->Points.size(); j++)
                          {
                            marks->SetNextPoint(EventsDataHub->Scan[iev]->Points[j].r[0], EventsDataHub->Scan[iev]->Points[j].r[1], EventsDataHub->Scan[iev]->Points[j].r[2]);
                            marks->SetNextPoint(EventsDataHub->Scan[iev]->Points[j].r[0], EventsDataHub->Scan[iev]->Points[j].r[1], EventsDataHub->Scan[iev]->zStop);
                            //showing track
                            Int_t track_index = MW->Detector->GeoManager->AddTrack(10,55); //  Here track_index is the index of the newly created track in the array of primaries. One can get the pointer of this track and make it known as current track by the manager class:
                            TVirtualGeoTrack *track = MW->Detector->GeoManager->GetTrack(track_index);
                            track->SetLineColor(kBlue);
                            track->AddPoint( EventsDataHub->Scan[iev]->Points[j].r[0], EventsDataHub->Scan[iev]->Points[j].r[1], EventsDataHub->Scan[iev]->Points[j].r[2], 0);
                            track->AddPoint( EventsDataHub->Scan[iev]->Points[j].r[0], EventsDataHub->Scan[iev]->Points[j].r[1], EventsDataHub->Scan[iev]->zStop, 0);
                          }
                      }
                    else
                      {
                        for (int j=0; j<EventsDataHub->Scan[iev]->Points.size(); j++)
                          marks->SetNextPoint(EventsDataHub->Scan[iev]->Points[j].r[0],
                                              EventsDataHub->Scan[iev]->Points[j].r[1],
                                              EventsDataHub->Scan[iev]->Points[j].r[2]);
                      }
                }

                everyCounter = 0;
                goodCounter++;
                if (goodCounter > UpperLimit) break;
            }
      }
    MW->GeoMarkers.append(marks);

    MW->GeometryWindow->show();
    MW->GeometryWindow->raise();
    MW->GeometryWindow->activateWindow();
    MW->ShowGeometry(false);
    MW->ShowTracks();
}

/*
void ReconstructionWindow::ShowReconstructionPositionsIfWindowVisible()
{  
  if (!MW->GeometryWindow->isVisible()) return;

  int CurrentGroup = PMgroups->getCurrentGroup();
  if (!EventsDataHub->isReconstructionReady(CurrentGroup)) return;


  int numEvents = EventsDataHub->ReconstructionData[CurrentGroup].size();
  if (numEvents==0) return;

  //preparing geometry window
  MW->Detector->GeoManager->ClearTracks(); //tracks could be used for indication (if continuous deposition)
  MW->clearGeoMarkers();

  //showing actual (if enabled) - if NOT on top
  //if (!ui->cbActualOnTop->isChecked()) ReconstructionWindow::DotActualPositions();

  //showing reconstruction
  int UpperLimit = numEvents;
  if (ui->cbLimitNumberEvents->isChecked()) UpperLimit = ui->sbLimitNumberEvents->value();

  GeoMarkerClass* marks = new GeoMarkerClass("Recon", 6, 2, kRed);

  int goodCounter = 0;
  int everyPeriod = ui->sbShowEventsEvery->value();
  int everyCounter = 0;
  for (int iev=0; iev<numEvents; iev++)
    {      
          if (EventsDataHub->ReconstructionData[CurrentGroup][iev]->GoodEvent)
            {
              everyCounter++;
              if (everyCounter == everyPeriod)
              {
                  for (int iP = 0; iP<EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points.size(); iP++)
                    marks->SetNextPoint(EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[iP].r[0],
                                        EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[iP].r[1],
                                        EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[iP].r[2]);
                  everyCounter = 0;
                  goodCounter++;
                  if (goodCounter > UpperLimit) break;
              }
            }
    }

  MW->GeoMarkers.append(marks);

  //showing actual (if enabled) - if on top
  //if (ui->cbActualOnTop->isChecked()) ReconstructionWindow::DotActualPositions();

  MW->ShowGeometry(false);
  MW->ShowTracks();
}
*/

void ReconstructionWindow::DotActualPositions()
{  
  //if (!ui->cbShowActualPosition->isChecked()) return;
  if (!EventsDataHub->isScanEmpty()) ReconstructionWindow::VisualizeScan();  
}

void ReconstructionWindow::VisualizeEnergyVector(int eventId)
{
  if (MW->EnergyVector.isEmpty()) return;

//  qDebug()<<"EnergyVector contains "<<MW->EnergyVector.size()<<" cells";

  QVector<AEnergyDepositionCell*> EV;
  EV.resize(0);
  for (int i=0; i<MW->EnergyVector.size(); i++)
    if (MW->EnergyVector.at(i)->eventId == eventId) EV.append(MW->EnergyVector.at(i));

//  qDebug()<<"This event ( "<< eventId <<" ) has "<<EV.size()<<" associated cells";

  if (EV.size() == 0) return;

  GeoMarkerClass* marks = new GeoMarkerClass("Scan", 6, 2, kBlue);

  int PreviousIndex = -1;
  bool flagTrackStarted = false;
  for (int ip=0; ip<EV.size(); ip++)
    {
      int ThisIndex = EV[ip]->index;
      if (ThisIndex == PreviousIndex)
        {
          //continuing the track
          if (!flagTrackStarted)
            {
              //previous was the first point of the track
//              qDebug()<<"  continuous deposition start detected";
              Int_t track_index = MW->Detector->GeoManager->AddTrack(10,22); //  Here track_index is the index of the newly created track in the array of primaries. One can get the pointer of this track and make it known as current track by the manager class:
              TVirtualGeoTrack *track = MW->Detector->GeoManager->GetTrack(track_index);
              MW->Detector->GeoManager->SetCurrentTrack(track);
              track->SetLineColor(kBlue);
              MW->Detector->GeoManager->GetCurrentTrack()->AddPoint(EV[ip-1]->r[0], EV[ip-1]->r[1], EV[ip-1]->r[2], 0);
            }
          flagTrackStarted = true; //nothing to do until track ended
        }
      else
        {
          //another particle starts here
          //track teminated last cell?
          if (flagTrackStarted)
            {
              //finishing the track of the previous particle
//              qDebug()<<"  continuous deposition end detected";
              MW->Detector->GeoManager->GetCurrentTrack()->AddPoint(EV[ip-1]->r[0], EV[ip-1]->r[1], EV[ip-1]->r[2], 0);
              //MW->DotsTGeo.append(DotsTGeoStruct(EV[ip-1]->r, kBlue));
              marks->SetNextPoint(EV[ip-1]->r[0], EV[ip-1]->r[1], EV[ip-1]->r[2]);
              flagTrackStarted = false;
            }
          //drawing this point - it is shown for both the track (start point) or individual
          //MW->DotsTGeo.append(DotsTGeoStruct(EV[ip]->r, kBlue));
          marks->SetNextPoint(EV[ip]->r[0], EV[ip]->r[1], EV[ip]->r[2]);

//          qDebug()<<EV[ip]->r[0]<<EV[ip]->r[1]<<EV[ip]->r[2];
        }
      PreviousIndex = ThisIndex;
    }

  //if finished, but track is not closed yet:
  if (flagTrackStarted)
    {
//      qDebug()<<"  continuous deposition end detected";
      MW->Detector->GeoManager->GetCurrentTrack()->AddPoint(EV.last()->r[0], EV.last()->r[1], EV.last()->r[2], 0);
      //MW->DotsTGeo.append(DotsTGeoStruct(EV.last()->r, kBlue));
      marks->SetNextPoint(EV.last()->r[0], EV.last()->r[1], EV.last()->r[2]);

//     qDebug()<<EV.last()->r[0]<< EV.last()->r[1]<< EV.last()->r[2];
    }

  MW->GeoMarkers.append(marks);
}

void ReconstructionWindow::VisualizeScan()
{  
  int CurrentGroup = PMgroups->getCurrentGroup();
  int numEvents = EventsDataHub->ReconstructionData[CurrentGroup].size();

  int UpperLimit = numEvents;
  if (ui->cbLimitNumberEvents->isChecked()) UpperLimit = ui->sbLimitNumberEvents->value();

  int goodCounter = 0;
  int everyPeriod = ui->sbShowEventsEvery->value();
  int everyCounter = 0;
  for (int iev=0; iev<EventsDataHub->ReconstructionData[CurrentGroup].size(); iev++)
    {
      if (EventsDataHub->ReconstructionData.at(CurrentGroup).at(iev)->GoodEvent)
        {
          everyCounter++;
          if (everyCounter == everyPeriod)
          {
              ReconstructionWindow::VisualizeScan(iev);

              everyCounter = 0;
              goodCounter++;
              if (goodCounter > UpperLimit) break;
          }
        }
    }
}

void ReconstructionWindow::PMnumChanged()
{
   MW->GraphWindow->LastDistributionShown = "";
   ui->sbPMsignalIndividual->setValue(0);
}

void ReconstructionWindow::on_sbPMsignalIndividual_valueChanged(int ipm)
{
  if (ipm == -1)
    {
      if (PMs->count() == 0) ui->sbPMsignalIndividual->setValue(0);
      else ui->sbPMsignalIndividual->setValue(PMs->count()-1);
      return; //update on_change
    }
  if (ipm > MW->PMs->count()-1)
    {
      if (ipm == 0) return;
      ui->sbPMsignalIndividual->setValue(0);
      return;
    }

  QString mi, ma;
  bool fIn;
  int current = PMgroups->getCurrentGroup();
  if (PMgroups->isPmInCurrentGroupFast(ipm))
  {
      mi.setNum( PMgroups->Groups.at(current)->PMS.at(ipm).cutOffMin );
      ma.setNum( PMgroups->Groups.at(current)->PMS.at(ipm).cutOffMax );
      fIn = true;
  }
  else
  {
      mi = ma = "n.a.";
      fIn = false;
  }

  ui->ledFilterIndividualMin->setText(mi);
  ui->ledFilterIndividualMax->setText(ma);
  ui->ledFilterIndividualMin->setEnabled(fIn);
  ui->ledFilterIndividualMax->setEnabled(fIn);

  if (MW->GraphWindow->LastDistributionShown == "IndiPMSignal") ReconstructionWindow::ShowIndividualSpectrum(false);
}

void ReconstructionWindow::on_pbCutOffsLoad_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this,
                                                  "Load individual cut-offs (ignores first column with PM number)",
                                                  MW->GlobSet->LastOpenDir, "Data files (*.dat);;Text files (*.txt)");

  if (fileName.isEmpty()) return;
  MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();

  LoadIndividualCutOffs(fileName);
  UpdateStatusAllEvents();  
}

void ReconstructionWindow::LoadIndividualCutOffs(QString fileName)
{
  QVector<double> ii, min, max;
  LoadDoubleVectorsFromFile(fileName, &ii, &min, &max);

  if (min.size() < MW->PMs->count())
    {
      message("Error: data do not exist for all PMs!", this);
      return;
    }

  int current = PMgroups->getCurrentGroup();
  for (int ipm=0; ipm<MW->PMs->count(); ipm++)
      PMgroups->setCutOffs( ipm, current, min[ipm], max[ipm], true);
  PMgroups->updateGroupsInGlobalConfig();
  UpdateStatusAllEvents();
  ReconstructionWindow::on_sbPMsignalIndividual_valueChanged(ui->sbPMsignalIndividual->value());  
}

void ReconstructionWindow::on_pbCutOffsSave_clicked()
{
  QString fileName = QFileDialog::getSaveFileName(this,
                                                  "Save individual cut-offs", MW->GlobSet->LastOpenDir, "Data files (*.dat);;Text files (*.txt)");

  if (fileName.isEmpty()) return;
  MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();

  ReconstructionWindow::SaveIndividualCutOffs(fileName);
}

void ReconstructionWindow::SaveIndividualCutOffs(QString fileName)
{
  int CurrentGroup = PMgroups->getCurrentGroup();
  QVector<double> ii, min, max;
  for (int ipm=0; ipm<MW->PMs->count(); ipm++)
    {
      ii.append(ipm);
      //min.append( MW->PMs->at(ipm).cutOffMin );
      min.append( PMgroups->getCutOffMin(ipm, CurrentGroup) );
      //max.append( MW->PMs->at(ipm).cutOffMax );
      max.append( PMgroups->getCutOffMax(ipm, CurrentGroup) );
    }
  SaveDoubleVectorsToFile(fileName, &ii, &min, &max);
}

void ReconstructionWindow::on_ledFilterIndividualMin_editingFinished()
{
   //MW->PMs->at(ui->sbPMsignalIndividual->value()).cutOffMin = ui->ledFilterIndividualMin->text().toDouble();
   int ipm = ui->sbPMsignalIndividual->value();
   int CurrentGroup = PMgroups->getCurrentGroup();
   PMgroups->setCutOffs(ipm, CurrentGroup, ui->ledFilterIndividualMin->text().toDouble(), PMgroups->getCutOffMax(ipm, CurrentGroup));
   UpdateStatusAllEvents();
}

void ReconstructionWindow::on_ledFilterIndividualMax_editingFinished()
{
   //MW->PMs->at(ui->sbPMsignalIndividual->value()).cutOffMax = ui->ledFilterIndividualMax->text().toDouble();
   int ipm = ui->sbPMsignalIndividual->value();
   int CurrentGroup = PMgroups->getCurrentGroup();
   PMgroups->setCutOffs(ipm, CurrentGroup, PMgroups->getCutOffMin(ipm, CurrentGroup), ui->ledFilterIndividualMax->text().toDouble());
   UpdateStatusAllEvents();
}

void ReconstructionWindow::on_pbShowSumSignal_clicked()
{
  int totNumEvents = EventsDataHub->Events.size();
  if ( totNumEvents == 0) return;

  auto hist1D = new TH1D("haSumSign","Sum signal",MW->GlobSet->BinsX, 0,0);
  hist1D->SetXTitle("Sum signal");

#if ROOT_VERSION_CODE < ROOT_VERSION(6,0,0)
  hist1D->SetBit(TH1::kCanRebin);
#endif

  int counter = 0;  
  double SumCutOffMin = ui->ledFilterSumMin->text().toDouble();
  double SumCutOffMax = ui->ledFilterSumMax->text().toDouble();

  bool AccountForPassive = ui->cbPassivePMsTakenAccount->isChecked();

  int current = PMgroups->getCurrentGroup();
  for (int iev = 0; iev < totNumEvents; iev++)
    if (EventsDataHub->ReconstructionData.at(current).at(iev)->GoodEvent)
    {
      double sum = 0;
      const QVector<float>* event = EventsDataHub->getEvent(iev);
      for (int ipm = 0; ipm < MW->PMs->count(); ipm++)
        if (PMgroups->isPmInCurrentGroupFast(ipm))
          //if (!MW->PMs->at(ipm).passive || AccountForPassive)
          if (PMgroups->isActive(ipm) || AccountForPassive)
          {
            double sig = event->at(ipm);
            if (ui->cbGainsConsideredInFiltering->isChecked()) sig /= PMgroups->Groups.at(current)->PMS.at(ipm).gain;
            sum += sig;
          }
      hist1D->Fill(sum);
      counter++;
    }

  if (counter > 0)
    {      
      MW->GraphWindow->Draw(hist1D);
        //showing range      
      double minY = 0;     
      double maxY = MW->GraphWindow->getCanvasMaxY();
      TLine* L1 = new TLine(SumCutOffMin, minY, SumCutOffMin, maxY);
      L1->SetLineColor(kGreen);
      L1->Draw();
      MW->GraphWindow->RegisterTObject(L1);
      TLine* L2 = new TLine(SumCutOffMax, minY, SumCutOffMax, maxY);
      L2->SetLineColor(kGreen);
      L2->Draw();
      MW->GraphWindow->RegisterTObject(L2);

      MW->GraphWindow->UpdateRootCanvas();
    }
  else message("There are no events which pass all the activated filters!", this);

  MW->GraphWindow->LastDistributionShown = "SumSignal";
}

void ReconstructionWindow::on_pbShowIndividualSpectrum_clicked()
{   
   ReconstructionWindow::ShowIndividualSpectrum(true);
}

void ReconstructionWindow::ShowIndividualSpectrum(bool focus)
{
  if (EventsDataHub->isEmpty()) return;

  int totNumEvents = EventsDataHub->Events.size();
  if ( totNumEvents == 0) return;

  int thisipm = ui->sbPMsignalIndividual->value();
  int CurrentGroup = PMgroups->getCurrentGroup();
  QString nameStr = "PM"+QString::number(thisipm)+" signal";
  QByteArray byteArray = nameStr.toUtf8();
  const char* name = byteArray.constData();

  TH1D* hist1D;
  double min = 0;
  double max = 0;
  if (ui->cbFilterIndividualSignals->isChecked())
    {
      min = PMgroups->getCutOffMin(thisipm, CurrentGroup);
      max = PMgroups->getCutOffMax(thisipm, CurrentGroup);

      if ( min==-1.0e10 || max==1.0e10)
        hist1D = new TH1D("haIndSpect", name, MW->GlobSet->BinsX, 0,0);
      else
        hist1D = new TH1D("haIndSpect", name, MW->GlobSet->BinsX, min,max);
    }
  else
      hist1D = new TH1D("haIndSpect", name, MW->GlobSet->BinsX, 0,0);

  hist1D->SetXTitle("Signal");

#if ROOT_VERSION_CODE < ROOT_VERSION(6,0,0)
  hist1D->SetBit(TH1::kCanRebin);
#endif

  int counter = 0;
  for (int iev = 0; iev < totNumEvents; iev++)
    {
      if (!EventsDataHub->ReconstructionData.at(0).at(iev)->GoodEvent) continue;
      double sig = EventsDataHub->getEvent(iev)->at(thisipm);
      if (min == max || (sig>min && sig<max))
        {
          hist1D->Fill(sig);
          counter++;
        }
    }

  if (counter > 0)
    {     
      if (focus) MW->GraphWindow->Draw(hist1D);
      else MW->GraphWindow->DrawWithoutFocus(hist1D);
       //showing range
      double minY = 0;     
      double maxY = MW->GraphWindow->getCanvasMaxY();      
      double cutOffMin = PMgroups->getCutOffMin(thisipm, CurrentGroup);
      double cutOffMax = PMgroups->getCutOffMax(thisipm, CurrentGroup);
      TLine* L1 = new TLine(cutOffMin, minY, cutOffMin, maxY);
      L1->SetLineColor(kGreen);
      L1->Draw();
      MW->GraphWindow->RegisterTObject(L1);
      TLine* L2 = new TLine(cutOffMax, minY, cutOffMax, maxY);
      L2->SetLineColor(kGreen);
      L2->Draw();
      MW->GraphWindow->RegisterTObject(L2);

      MW->GraphWindow->UpdateRootCanvas();
    }
  else message("There are no events which pass all activated filters!", this);

  MW->GraphWindow->LastDistributionShown = "IndiPMSignal";
}

void ReconstructionWindow::SetGuiBusyStatusOn()
{
    MW->WindowNavigator->BusyOn();
}

void ReconstructionWindow::SetGuiBusyStatusOff()
{
    MW->WindowNavigator->BusyOff();
}

void ReconstructionWindow::on_pbGoToNextEvent_clicked()
{
    if (EventsDataHub->isEmpty()) return;

    int CurrentGroup = PMgroups->getCurrentGroup();
    if (!EventsDataHub->isReconstructionReady(CurrentGroup))
      {
        message("To use this feature, first perform \"Reconstruct all events\"", this);
        return;
      }

    QString selector = ui->cobGoToSelector->currentText();


    for (int iev = ui->sbEventNumberInspect->value()+1; iev < EventsDataHub->Events.size(); iev++)
      {              
        const AReconRecord* result = EventsDataHub->ReconstructionData[CurrentGroup][iev];

        //good event
        if (result->GoodEvent && selector == "good event")
         {
           ui->sbEventNumberInspect->setValue(iev);
           return;
         }
        //bad event
        if (!result->GoodEvent && selector == "bad event")
         {
           ui->sbEventNumberInspect->setValue(iev);
           return;
         }
        //failed reconstruction
        if (!result->ReconstructionOK && selector == "failed reconstruction")
         {
             ui->sbEventNumberInspect->setValue(iev);
             return;
         }
      }
    message("No more events according to the selected criterium!", this);
}

void ReconstructionWindow::on_pbShowMeasuredVsExpected_clicked()
{
    int CurrentGroup = PMgroups->getCurrentGroup();
    if (EventsDataHub->isEmpty())
      {
        message("Data are not selected!", this);
        ui->twData->setCurrentIndex(0);
        return;
      }
    if (!Detector->LRFs->isAllLRFsDefined())
      {
        message("LRFs are not defined!", this);
        ui->twOptions->setCurrentIndex(1);
        return;
      }
    //redo reconstruction:
    int iev = ui->sbEventNumberInspect->value();
    if (iev > EventsDataHub->Events.size()-1)
      {
        qDebug()<<"Bad event number in reconstruct one event!";
        return;
      }
    AReconRecord* rec = EventsDataHub->ReconstructionData[CurrentGroup][iev];
    if (!rec->ReconstructionOK)
      {
        message("Reconstruction failed, no data to show!", this);
        return;
      }

    DynamicPassivesHandler *Passives = new DynamicPassivesHandler(PMs, PMgroups, EventsDataHub);
    if (CurrentGroup > EventsDataHub->RecSettings.size()-1)
       Passives->init(0, CurrentGroup);
    else
    {
        Passives->init(&EventsDataHub->RecSettings[CurrentGroup], CurrentGroup);
        if (EventsDataHub->RecSettings.at(CurrentGroup).fUseDynamicPassives) Passives->calculateDynamicPassives(iev, rec);
    }

    QVector<double> xA, yA, xdP, ydP;
    for (int ipm = 0; ipm<MW->PMs->count(); ipm++)
      {
        //if (!MW->PMs->at(ipm).passive)
        if (Passives->isActive(ipm))
          {
            //active PM
            double ExpValue = 0;
            for (int iP=0; iP<rec->Points.size(); iP++)
            ExpValue += Detector->LRFs->getLRF(ipm, rec->Points[iP].r)*rec->Points[iP].energy;

            xA.append(ExpValue);          
            yA.append(EventsDataHub->Events[iev][ipm]);
          }
        //else if (MW->PMs->at(ipm).isDynamicPassive())
        else if (Passives->isDynamicPassive(ipm))
          {
            //dynamic passive PM
            double ExpValue = 0;
            for (int iP=0; iP<rec->Points.size(); iP++)
            ExpValue += Detector->LRFs->getLRF(ipm, rec->Points[iP].r)*rec->Points[iP].energy;

            xdP.append(ExpValue);          
            ydP.append(EventsDataHub->Events[iev][ipm]);
          }
      }

    if (xdP.isEmpty()) MW->GraphWindow->MakeGraph(&xA, &yA, kRed, "Expected (from LRF)", "PM signal", 20, 1, 0, 0);
    else
      {
        TGraph* actives = MW->GraphWindow->MakeGraph(&xA, &yA, kRed, "Expected (from LRF)", "PM signal", 20, 1, 0, 0, "", true);
        TGraph* passives = MW->GraphWindow->MakeGraph(&xdP, &ydP, kGray, "Expected (from LRF)", "PM signal", 20, 1, 0, 0, "", true);

        TMultiGraph* multigrEE = new TMultiGraph();
        multigrEE->Add(passives, "P"); //on delete, multigraph automatically deletes member graphs
        multigrEE->Add(actives, "P");
        multigrEE->SetTitle("Measured vs Expected PM signals;Expected signal (from LRF);PM signal");

        MW->GraphWindow->Draw(multigrEE, "A");
      }

    MW->GraphWindow->LastDistributionShown = "ShowExpectedVsMeasured";
    delete Passives;
}

void ReconstructionWindow::on_pbConfigureNN_clicked()
{
#ifdef ANTS_FANN
   MW->NNwindow->showNormal();
   MW->NNwindow->activateWindow();
#else
   message("Neural networks were disabled in .pro", this);
#endif
}

void ReconstructionWindow::LRF_ModuleReadySlot(bool ready)
{ 
    //since there are two modules, ignore the arriving bool and asking the current status from LRF selector
    ready = Detector->LRFs->isAllLRFsDefined();
//   qDebug()<<ready;
    QString status;
    if (ready) status = "Ready";
    else status = "Not Ready";

    ui->label_ready->setText(status);
    if (ready)
       {
        ui->label_ready->setStyleSheet("QLabel { color : green; }");
        QIcon no;
        ui->twOptions->tabBar()->setTabIcon(1, no);
       }
    else
      {
        ui->label_ready->setStyleSheet("QLabel { color : red; }");
        ui->twOptions->tabBar()->setTabIcon(1, RedIcon);
      }
    ReconstructionWindow::updateRedStatusOfRecOptions();

    MW->LRF_ModuleReadySlot(ready);
}

void ReconstructionWindow::updateRedStatusOfRecOptions()
{
  bool showRed = false;

  int recAlg = ui->cobReconstructionAlgorithm->currentIndex();
  if (!Detector->LRFs->isAllLRFsDefined())
       if (recAlg !=0 && recAlg !=3) showRed = true; //CoG and network do not need LRFs!

  if (showRed)
    {
      ui->label_ready->setStyleSheet("QLabel { color : red; }");
      ui->twOptions->tabBar()->setTabIcon(1, RedIcon);
      ui->tabWidget->tabBar()->setTabIcon(1, RedIcon);
    }
  else
    {
      QIcon no;
      ui->tabWidget->tabBar()->setTabIcon(1, no);
    }
}

void ReconstructionWindow::updateFiltersGui()
{
  //Update UI
  ui->fCustomSpatFilter->setEnabled(ui->cbSpFcustom->isChecked());
  ui->fSpFz->setEnabled(!ui->cbSpFallZ->isChecked());
    //warning icons
  bool masterWarningFlag = false;
  QIcon no;
  if (ui->cbFilterEventNumber->isChecked() || ui->cbFilterMultipleScanEvents->isChecked())
    {
      ui->twData->tabBar()->setTabIcon(0, YellowIcon);
      //ui->twData->tabBar()->setTabTextColor(0, QColor(237, 160, 59));
      //masterWarningFlag = true;
    }
  else
    {
      //ui->twData->tabBar()->setTabTextColor(0, Qt::black);
      ui->twData->tabBar()->setTabIcon(0, no);
    }
  if (ui->cbFilterSumSignal->isChecked() || ui->cbFilterIndividualSignals->isChecked())
    {
      ui->twData->tabBar()->setTabIcon(1, YellowIcon);
      masterWarningFlag = true;
    }
  else ui->twData->tabBar()->setTabIcon(1, no);
  if (ui->cbActivateEnergyFilter->isChecked() || ui->cbActivateLoadedEnergyFilter->isChecked())
    {
      ui->twData->tabBar()->setTabIcon(2, YellowIcon);
      masterWarningFlag = true;
    }
  else ui->twData->tabBar()->setTabIcon(2, no);
  if (ui->cbActivateChi2Filter->isChecked())
    {
      ui->twData->tabBar()->setTabIcon(3, YellowIcon);
      masterWarningFlag = true;
    }
  else ui->twData->tabBar()->setTabIcon(3, no);
  if (ui->cbSpF_LimitToObject->isChecked() || ui->cbSpFcustom->isChecked())
    {
      ui->twData->tabBar()->setTabIcon(4, YellowIcon);
      masterWarningFlag = true;
    }
  else ui->twData->tabBar()->setTabIcon(4, no);
  if (ui->cbActivateCorrelationFilters->isChecked())
    {
      ui->twData->tabBar()->setTabIcon(5, YellowIcon);
      masterWarningFlag = true;
    }
  else ui->twData->tabBar()->setTabIcon(5, no);
  if (ui->cbActivateNNfilter->isChecked())
    {
      ui->twData->tabBar()->setTabIcon(6, YellowIcon);
      masterWarningFlag = true;
    }
  else ui->twData->tabBar()->setTabIcon(6, no);

  if (masterWarningFlag) ui->tabWidget->tabBar()->setTabIcon(0, YellowIcon);
  else ui->tabWidget->tabBar()->setTabIcon(0, no);

    //building polygon if ui->cobSpFXY->currentIndex() != 3 (custom), otherwise reading the table
  int mode = ui->cobSpFXY->currentIndex();
  ui->fTabPoly->setVisible(mode == 3);
  double x0 = ui->ledSpF_X0->text().toDouble();
  double y0 = ui->ledSpF_Y0->text().toDouble();
  switch (mode)
    {
     case 0: //square
      {
        polygon.clear();
//          qDebug()<<"creating polygon: square";
      double xs = ui->ledSpFsizeX->text().toDouble()*0.5;
      double ys = ui->ledSpFsizeY->text().toDouble()*0.5;      
      polygon << QPointF(x0+xs , y0+ys) << QPointF(x0+xs, y0-ys) << QPointF(x0-xs, y0-ys) << QPointF(x0-xs, y0+ys) << QPointF(x0+xs, y0+ys);
//        qDebug()<<"points:"<<polygon.size();
      break;
      }
     case 1: //round
      {
        polygon.clear();
       double radius = ui->ledSpFdiameter->text().toDouble()*0.5;
       for (int i=0; i<51; i++)
         {
           double angle = 3.1415926536*i/25.0;
           polygon << QPointF(x0+radius*sin(angle), y0+radius*cos(angle));
         }
       break;
      }
     case 2: //hexagon
      {
       polygon = MakeClosedPolygon(ui->ledSpFside->text().toDouble(), ui->ledSpFangle->text().toDouble(), x0, y0);
       break;
      }
     case 3:
      {
        //reading table and creating polygon
        ReconstructionWindow::TableToPolygon();
      }
    }

  //writing the table
  ReconstructionWindow::on_pbSpF_UpdateTable_clicked();  
}

void ReconstructionWindow::on_pbUpdateFilters_clicked()
{
  //qDebug() << "UpdateFilterButton pressed";
  if (bFilteringStarted) //without this on-Editing-finished is triggered when disable kick in and cursor is in one of the edit boxes
  {
      //qDebug() << "Igonred, already filetring";
      return;
  }

  bFilteringStarted = true;  //--in--//
  MW->WindowNavigator->BusyOn();

  ReconstructionWindow::updateFiltersGui();
  if (ui->cbSpFcustom->isChecked() && ui->twData->tabText(ui->twData->currentIndex()) == "Spatial")
    if (MW->GeometryWindow->isVisible()) on_pbShowSpatialFilter_clicked();

  if (TMPignore) UpdateReconConfig(); // in this case only GUI update and Config->JSON update
  else UpdateStatusAllEvents();

  MW->WindowNavigator->BusyOff();
  bFilteringStarted = false; //--out--//
}

void ReconstructionWindow::on_pbEnergySpectrum1_clicked()
{
    ReconstructionWindow::ShowEnergySpectrum();
}

void ReconstructionWindow::on_pbEnergySpectrum2_clicked()
{
    ReconstructionWindow::ShowEnergySpectrum();   
}

void ReconstructionWindow::ShowEnergySpectrum()
{
    int CurrentGroup = PMgroups->getCurrentGroup();
    if (!EventsDataHub->isReconstructionReady(CurrentGroup))
      {
        message("Reconstruction was not yet performed!", this);
        return;
      }
    if (EventsDataHub->countGoodEvents(CurrentGroup) == 0)
      {
        message("There are no events passing all activated filters", this);
        return;
      }

    auto hist1D = new TH1D("hist1ShEnSp","Energy spectrum", MW->GlobSet->BinsX, 0, 0);

#if ROOT_VERSION_CODE < ROOT_VERSION(6,0,0)
    hist1D->SetBit(TH1::kCanRebin);
#endif

    for (int i=0; i<EventsDataHub->ReconstructionData[CurrentGroup].size(); i++)
        if (EventsDataHub->ReconstructionData[CurrentGroup][i]->GoodEvent)
          for (int iP=0; iP<EventsDataHub->ReconstructionData[CurrentGroup][i]->Points.size(); iP++)
            hist1D->Fill(EventsDataHub->ReconstructionData[CurrentGroup][i]->Points[iP].energy);

    hist1D->SetXTitle("Energy");
    MW->GraphWindow->Draw(hist1D);
    //showing range
    double minY = 0;
    //double maxY = MW->graphRW->getCanvasMaxY();
    double maxY = MW->GraphWindow->getCanvasMaxY();
    double filtMin = ui->ledEnergyMin->text().toDouble();
    double filtMax = ui->ledEnergyMax->text().toDouble();
    TLine* L1 = new TLine(filtMin, minY, filtMin, maxY);
    L1->SetLineColor(kGreen);
    L1->Draw();
    MW->GraphWindow->RegisterTObject(L1);
    TLine* L2 = new TLine(filtMax, minY, filtMax, maxY);
    L2->SetLineColor(kGreen);
    L2->Draw();
    MW->GraphWindow->RegisterTObject(L2);

    MW->GraphWindow->UpdateRootCanvas();    
    MW->GraphWindow->LastDistributionShown = "EnergySpectrum";
}

void ReconstructionWindow::on_pbChi2Spectrum1_clicked()
{  
  ReconstructionWindow::ShowChi2Spectrum();
}

void ReconstructionWindow::on_pbChi2Spectrum2_clicked()
{
    ReconstructionWindow::ShowChi2Spectrum();
}

void ReconstructionWindow::ShowChi2Spectrum()
{
  int CurrentGroup = PMgroups->getCurrentGroup();
  if (!EventsDataHub->isReconstructionReady(CurrentGroup))
    {
      message("Reconstruction was not yet performed!", this);
      return;
    }
  if (EventsDataHub->countGoodEvents(CurrentGroup) == 0)
    {
      message("There are no events passing all activated filters", this);
      return;
    }

  auto hist1D = new TH1D("hist1Chi2Dis","Chi2 distribution", MW->GlobSet->BinsX, 0, 0);
  hist1D->SetXTitle("Chi2");
#if ROOT_VERSION_CODE < ROOT_VERSION(6,0,0)
  hist1D->SetBit(TH1::kCanRebin);
#endif

  for (int i=0; i<EventsDataHub->ReconstructionData[CurrentGroup].size(); i++)
      if (EventsDataHub->ReconstructionData[CurrentGroup][i]->GoodEvent)
          hist1D->Fill(EventsDataHub->ReconstructionData[CurrentGroup ][i]->chi2);

  MW->GraphWindow->Draw(hist1D);
  double minY = 0;
  double maxY = MW->GraphWindow->getCanvasMaxY();
  double filtMin = ui->ledChi2Min->text().toDouble();
  double filtMax = ui->ledChi2Max->text().toDouble();
  TLine* L1 = new TLine(filtMin, minY, filtMin, maxY);
  L1->SetLineColor(kGreen);
  L1->Draw();
  MW->GraphWindow->RegisterTObject(L1);
  TLine* L2 = new TLine(filtMax, minY, filtMax, maxY);
  L2->SetLineColor(kGreen);
  L2->Draw();
  MW->GraphWindow->RegisterTObject(L2);

  MW->GraphWindow->UpdateRootCanvas();
  MW->GraphWindow->LastDistributionShown = "Chi2Spectrum";
}

void ReconstructionWindow::ShowFiltering()
{
   ui->tabWidget->setCurrentIndex(0);
   ui->bsAnalyzeScan->setCurrentIndex(0);
}

void ReconstructionWindow::ResetStop()
{
  SetProgress(0);
  ui->pbStopReconstruction->setChecked(false);
  ui->pbStopReconstruction->setEnabled(false);
}

void ReconstructionWindow::SetProgress(int val)
{
  ui->prReconstruction->setValue(val);
  MW->WindowNavigator->setProgress(val);
}

void ReconstructionWindow::onBusyOn()
{
  //qDebug() << "Busy ON!";
  ui->twData->setEnabled(false);
  ui->twOptions->setEnabled(false);
  ui->bsAnalyzeScan->setEnabled(false);
}

void ReconstructionWindow::onBusyOff()
{
  //qDebug() << "Busy OFF!";
  ui->twData->setEnabled(true);
  ui->twOptions->setEnabled(true);
  ui->bsAnalyzeScan->setEnabled(true);
  ui->pbStopReconstruction->setEnabled(false);
  ui->pbStopReconstruction->setChecked(false);
}

void ReconstructionWindow::on_sbZshift_valueChanged(int arg1)
{
  QString str;
  int delta = arg1-ShiftZoldValue;

  str.setNum(ui->ledZfrom->text().toDouble()+delta);
  ui->ledZfrom->setText(str);
  str.setNum(ui->ledZto->text().toDouble()+delta);
  ui->ledZto->setText(str);

  ShiftZoldValue = arg1;
}

void ReconstructionWindow::on_cobHowToAverageZ_currentIndexChanged(int index)
{
  ui->fAverageZ->setVisible((bool)index);
}

bool ReconstructionWindow::event(QEvent *event)
{
  if (!MW->WindowNavigator) return QMainWindow::event(event);

  if (event->type() == QEvent::Hide)
    {      
      MW->WindowNavigator->HideWindowTriggered("recon");
      return true;
    }
  if (event->type() == QEvent::Show)
    {
      MW->WindowNavigator->ShowWindowTriggered("recon");
    }

  return QMainWindow::event(event);
}

bool ReconstructionWindow::ShowVsXY(QString strIn) //false - error
{    
  if (EventsDataHub->isEmpty())
    {
      message("There are no data!", this);
      return false;
    }

  int CurrentGroup = PMgroups->getCurrentGroup();
  if (!EventsDataHub->isReconstructionReady(CurrentGroup))
    {
      if (ui->cbPlotVsActualPosition->isChecked() && (strIn=="PM signal" || strIn=="Sum PM signal" ) )
        {
           //in this case reconstruction is not needed
        }
      else
        {
          message("Data reconstruction was not performed yet!", this);
          return false;
        }
    }

  if (EventsDataHub->isScanEmpty())
    {
      if (strIn.startsWith("Delta rec-true") || strIn.startsWith("Sigma rec-true") || ui->cbPlotVsActualPosition->isChecked())
       {
          message("There are no true data!", this);
          return false;
       }
    }

  int ipmSelected = ui->sbShowXYpmNumber->value();
  if (strIn == "PM signal")
    {
       if (ipmSelected > MW->PMs->count()-1)
       {
           message("PM number is out of range!", this);
           return false;
       }
    }

  bool AccountForPassive = ui->cbPassivePMsTakenAccount->isChecked();

  //init
  const double Zmin = ui->ledZfrom->text().toDouble();
  const double Zmax = ui->ledZto->text().toDouble();
  const bool UseScan = ui->cbPlotVsActualPosition->isChecked();
  const bool fShowPMs = ui->cbShowPMs->isChecked();
  const bool fShowManifest = ui->cbShowManifestItems->isChecked();
  const bool bInvertX = ui->cbInvertX->isChecked();
  const bool bInvertY = ui->cbInvertY->isChecked();

  auto hist2D = new TH2D("hist2d","",ui->sbXbins->value(), ui->ledXfrom->text().toDouble(), ui->ledXto->text().toDouble(),
                                    ui->sbYbins->value(), ui->ledYfrom->text().toDouble(), ui->ledYto->text().toDouble());

//  qDebug() << "Events:" <<  EventsDataHub->ReconstructionData.size();
  enum ModeEnum {Energy, Chi2, Density, SumSignal, PMsignal, DeltaXY, DeltaX, DeltaY};
  ModeEnum Mode;
  if (strIn == "Energy") Mode = Energy;
  else if (strIn == "Chi2") Mode = Chi2;
  else if (strIn == "Event density") Mode = Density;
  else if (strIn == "Sum PM signal") Mode = SumSignal;
  else if (strIn == "PM signal") Mode = PMsignal;
  else if (strIn == "Delta rec-true XY") Mode = DeltaXY;
  else if (strIn == "Delta rec-true X") Mode = DeltaX;
  else if (strIn == "Delta rec-true Y") Mode = DeltaY;
  else if (strIn == "Res X")
    {
      qDebug() << "Doing res X estimation";
      Mode = DeltaX;
      hist2D->Sumw2(true);
    }
  else if (strIn == "Res Y")
    {
      Mode = DeltaY;
      hist2D->Sumw2(true);
    }
  else
    {
      message("Error: unknow distribution requested!", this);
      return false;
    }

  //hist2D will hold the sum of energy,
  //while histNum will have number of events contributed to each cell
  TH2D* histNum = new TH2D("histNum","h2",ui->sbXbins->value(), ui->ledXfrom->text().toDouble(), ui->ledXto->text().toDouble(),
                                        ui->sbYbins->value(), ui->ledYfrom->text().toDouble(), ui->ledYto->text().toDouble());

  for (int iev = 0; iev < EventsDataHub->ReconstructionData[CurrentGroup].size(); iev++)
   {
    if (EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points.size() == 1)
      {
        if (! EventsDataHub->ReconstructionData[CurrentGroup][iev]->GoodEvent) continue;

        if (ui->cobHowToAverageZ->currentIndex() == 1)
          {//have to filter by Z
            double Z;
            if (UseScan) Z = EventsDataHub->Scan[iev]->Points[0].r[2];
            else Z = EventsDataHub->ReconstructionData[0][iev]->Points[0].r[2];

            if (Z<Zmin || Z>Zmax) continue;
          }

        double val;        
        switch (Mode)
        {
        case Energy:    val = EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].energy; break;
        case Chi2:      val = EventsDataHub->ReconstructionData[CurrentGroup][iev]->chi2; break;
        case Density:   val = 1.0; break;
        case SumSignal:
          {
            val = 0;
            for (int ipm = 0; ipm<PMs->count(); ipm++)
             if (PMgroups->isPmInCurrentGroupFast(ipm))
              //if (!PMs->at(ipm).passive || AccountForPassive)
              if (PMgroups->isActive(ipm) || AccountForPassive)
                {
                  double delta = EventsDataHub->Events[iev][ipm];
                  if (ui->cbGainsConsideredInFiltering->isChecked()) delta /= PMgroups->Groups.at(CurrentGroup)->PMS.at(ipm).gain;
                  val += delta;
                }
            break;
          }
        case PMsignal: val = EventsDataHub->Events[iev][ipmSelected]; break;
        case DeltaXY:
          {
            double deltaX = EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[0] - EventsDataHub->Scan[iev]->Points[0].r[0]; deltaX *= deltaX;
            double deltaY = EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[1] - EventsDataHub->Scan[iev]->Points[0].r[1]; deltaY *= deltaY;
            val = TMath::Sqrt(deltaX + deltaY);
            break;
          }
        case DeltaX:   val = EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[0] - EventsDataHub->Scan[iev]->Points[0].r[0]; break;
        case DeltaY:   val = EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[1] - EventsDataHub->Scan[iev]->Points[0].r[1]; break;
        default:;
        }

        double X, Y;
        if (UseScan)
          {
             X = EventsDataHub->Scan[iev]->Points[0].r[0];
             Y = EventsDataHub->Scan[iev]->Points[0].r[1];
          }
        else
          {
             X = EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[0];
             Y = EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[1];
          }
        if (bInvertX) X *= -1.0;
        if (bInvertY) Y *= -1.0;

        hist2D->Fill( X, Y, val );
        histNum->Fill( X, Y );
      }
    else qWarning() << "Not implemented for multiple points!";

   }

  //calculating average
  if (strIn != "Event density" && !strIn.startsWith("Res"))
      *hist2D = *hist2D / *histNum;

  //estimating resolution
  if (strIn.startsWith("Res"))
  {
      // sigma calculation
      // sigma2 = 1/(N-1) * Sum( (x-a)^2 ) = 1/(N-1) * ( Sum(x^2) -2aSum(X) + a*a*N ) = 1/(N-1) * ( Sum(x^2) -a*a*N )
      int numBinsX = hist2D->GetNbinsX();
      int numBinsY = hist2D->GetNbinsY();
      const int thresholdEntriesPerBin = 5; //ignore bins with less than 5 entries
      for (int ix = 1; ix<numBinsX+1; ix++)
          for (int iy = 1; iy<numBinsY+1; iy++)
          {
              double val = 0;
              int N = histNum->GetBinContent(ix, iy);
              if (N >= thresholdEntriesPerBin)
                {
                  double sum_x = hist2D->GetBinContent(ix, iy);
                  double a = sum_x / N;
                  double RootErr = hist2D->GetBinError(ix, iy); //this is sqrt of Sum( x^2 )
                  double sigma2 = 1.0/(N-1) * (RootErr*RootErr - a*a*N);
                  double sigma = sqrt(sigma2);
                  val = 2.35482*sigma;
                }
              hist2D->SetBinContent(ix, iy, val);
          }
  }

  //title and axes
  if (strIn == "Event density")         hist2D->SetTitle("Event density vs XY");
  else if (strIn == "Energy")           hist2D->SetTitle("Event energy vs XY");
  else if (strIn == "Chi2")             hist2D->SetTitle("Chi2 vs XY");
  else if (strIn == "Sum PM signal")    hist2D->SetTitle("Sum PM signal vs XY");
  else if (strIn == "Delta rec-true XY")hist2D->SetTitle("Delta reconstructed-true R vs XY");
  else if (strIn == "Delta rec-true X") hist2D->SetTitle("Delta reconstructed-true X vs XY");
  else if (strIn == "Delta rec-true Y") hist2D->SetTitle("Delta reconstructed-true Y vs XY");
  else if (strIn == "Res X")            hist2D->SetTitle("Resolution in X vs XY");
  else if (strIn == "Res Y")            hist2D->SetTitle("Resolution in Y vs XY");
  else if (strIn == "PM signal")        hist2D->SetTitle("PM signal vs XY");

  hist2D->GetXaxis()->SetTitle("X, mm");
  hist2D->GetYaxis()->SetTitle("Y, mm");

  double max =  hist2D->GetMaximum();
  double min =  hist2D->GetMinimum();
  double min0 = hist2D->GetMinimum(0);  //except 0
  //qDebug()<<"min/max"<<min << max << "min0"<<min0;
  if (ui->cbSuppress0bins->isChecked() && strIn != "Delta rec-true X" && strIn != "Delta rec-true Y")
        hist2D->GetZaxis()->SetRangeUser(min0, max);
  else
    {
      if (min >= 0) hist2D->GetZaxis()->SetRangeUser(-0.01, max);
      else          hist2D->GetZaxis()->SetRangeUser(min, max);
    }

  //drawing
  MW->GraphWindow->Draw(hist2D, "colz", !(fShowPMs || fShowManifest) );
  delete histNum;

  //show additional objects
  if (fShowPMs)
    {
      for (int ipm=0; ipm<MW->PMs->count(); ipm++)
        {
          double x = MW->PMs->X(ipm);
          double y = MW->PMs->Y(ipm);
          double r1 = 0.5*MW->PMs->getTypeForPM(ipm)->SizeX;
          double r2 = 0.5*MW->PMs->getTypeForPM(ipm)->SizeY;
          int shape = MW->PMs->getTypeForPM(ipm)->Shape; // 0 - circ, 1 - rect, 2 - hexa

          switch (shape)
            {
            case 0:
              {
                TBox* e = new TBox(x-r1, y-r2, x+r1, y+r2);
                e->SetFillStyle(0);
                e->SetLineColor(PMcolor);
                e->SetLineWidth(PMwidth);
                e->SetLineStyle(PMstyle);
                MW->GraphWindow->Draw(e, "same", false);
                break;
              }
            case 1:
              {
                TEllipse* e = new TEllipse(x, y, r1, r1);
                e->SetFillStyle(0);
                e->SetLineColor(PMcolor);
                e->SetLineWidth(PMwidth);
                e->SetLineStyle(PMstyle);
                MW->GraphWindow->Draw(e, "same", false);
                break;
              }
            case 2:
              {
                double xx[7];
                double yy[7];
                for (int j=0; j<7; j++)
                  {
                    double angle = 3.1415926535/3.0 * j + 3.1415926535/2.0;
                    xx[j] = x + r1*cos(angle);
                    yy[j] = y + r1*sin(angle);
                  }
                TPolyLine *e = new TPolyLine(7, xx, yy);           
                e->SetFillStyle(0);
                e->SetLineColor(PMcolor);
                e->SetLineWidth(PMwidth);
                e->SetLineStyle(PMstyle);
                MW->GraphWindow->Draw(e, "same", false);
              }
            }          
        }      
    }

  if (fShowManifest)
    {
      for (int iobj=0; iobj<EventsDataHub->Manifest.size(); iobj++)
        {
          TObject* ob = EventsDataHub->Manifest[iobj]->makeTObject();
          MW->GraphWindow->Draw(ob, "same", false);
        }
    }

  if (fShowPMs || fShowManifest)
    {
      MW->GraphWindow->UpdateRootCanvas();
      MW->GraphWindow->UpdateControls();
    }

  MW->GraphWindow->LastDistributionShown = "ShowVsXY";
  return true;
}

void ReconstructionWindow::on_pbShowSpatialFilter_clicked()
{  
    MW->Detector->GeoManager->ClearTracks();
    MW->GeometryWindow->SetAsActiveRootWindow();

    if (ui->cbSpFallZ->isChecked()) ReconstructionWindow::DrawPolygon(0);
    else
      {
        ReconstructionWindow::DrawPolygon(ui->ledSpFfromZ->text().toDouble());
        ReconstructionWindow::DrawPolygon(ui->ledSpFtoZ->text().toDouble());
      }
    MW->ShowTracks();
}

void ReconstructionWindow::DrawPolygon(double z)
{
  Int_t track_index = MW->Detector->GeoManager->AddTrack(11,22); //  user id = 11 for filter
  TVirtualGeoTrack *track = MW->Detector->GeoManager->GetTrack(track_index);
  track->SetLineColor(kBlue);
  track->SetLineWidth(3);

  for (int i=0; i<polygon.size(); i++)
    {
      double x = polygon[i].x();
      double y = polygon[i].y();
//      qDebug()<<x<<y;
      track->AddPoint(x, y, z, 0);
    }
//  qDebug()<<"track added";
}

void ReconstructionWindow::on_pbSpF_UpdateTable_clicked()
{
   ReconstructionWindow::PolygonToTable();
}

void ReconstructionWindow::on_pbSpF_AddLine_clicked()
{ //add line before
  int selected = ui->tabwidSpF->currentRow();
//  qDebug()<<selected;

  polygon.insert(selected, QPointF(0.0, 0.0));
  if (selected == 0)
    {
      //have to replace the last node (hidden) too
      polygon.replace(polygon.size()-1, polygon.first());
    }
  ReconstructionWindow::PolygonToTable();

  if (ui->cobSpFXY->currentIndex() != 3) ui->cobSpFXY->setCurrentIndex(3);
  else ReconstructionWindow::on_pbUpdateFilters_clicked();
}

void ReconstructionWindow::on_pbSpF_AddLineAfter_clicked()
{ //add line after
  int selected = ui->tabwidSpF->currentRow();
//  qDebug()<<selected;

  polygon.insert(selected+1, QPointF(0.0, 0.0));
  ReconstructionWindow::PolygonToTable();

  if (ui->cobSpFXY->currentIndex() != 3) ui->cobSpFXY->setCurrentIndex(3);
  else ReconstructionWindow::on_pbUpdateFilters_clicked();
}

void ReconstructionWindow::on_pbSpF_RemoveLine_clicked()
{
    int selected = ui->tabwidSpF->currentRow();
//    qDebug()<<selected;
    if (selected == -1)
      {
        message("select a row first!", this);
        return;
      }
    if (ui->tabwidSpF->rowCount()<4)
      {
        message("Minimum is three rows!", this);
        return;
      }

    polygon.remove(selected);
    if (selected == 0)
      {
        //have to change the last point of polygon too (it is not shown in the table)
        polygon.remove(polygon.size()-1);
        polygon.append(polygon.first());
      }
    ReconstructionWindow::PolygonToTable();

    if (ui->cobSpFXY->currentIndex() != 3) ui->cobSpFXY->setCurrentIndex(3);
    else ReconstructionWindow::on_pbUpdateFilters_clicked();
}

void ReconstructionWindow::on_tabwidSpF_cellChanged(int row, int column)
{
   if (TMPignore) return;
   if (TableLocked) return; //do not update during filling with data!

   bool ok;
   QString str = ui->tabwidSpF->item(row,column)->text();
   str.toDouble(&ok);
   if (!ok)
     {
       message("Input error!", this);
       ReconstructionWindow::PolygonToTable();
       return;
     }   

   if (ui->cobSpFXY->currentIndex() != 3) ui->cobSpFXY->setCurrentIndex(3);
   else ReconstructionWindow::on_pbUpdateFilters_clicked();
}

void ReconstructionWindow::PolygonToTable()
{
  //table inits
  TableLocked = true;
  ui->tabwidSpF->clearContents();    
  ui->tabwidSpF->setShowGrid(false);
  int rows = polygon.size()-1;
  if (polygon.isEmpty() || ui->cobSpFXY->currentIndex() == 1) rows = 0;
  ui->tabwidSpF->setRowCount(rows);
  ui->tabwidSpF->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tabwidSpF->setSelectionMode(QAbstractItemView::SingleSelection);

  for (int j=0; j<ui->tabwidSpF->rowCount(); j++)
    {
      QString str;
      str.setNum(polygon[j].x(), 'f', 1);
      ui->tabwidSpF->setItem(j, 0, new QTableWidgetItem(str));
      str.setNum(polygon[j].y(), 'f', 1);
      ui->tabwidSpF->setItem(j, 1, new QTableWidgetItem(str));
    }

  ui->tabwidSpF->resizeColumnsToContents();
  ui->tabwidSpF->resizeRowsToContents();
  //ui->tabwidSpF->setColumnWidth(0, 56);
  //ui->tabwidSpF->setColumnWidth(1, 56);
  TableLocked = false;
}

void ReconstructionWindow::TableToPolygon()
{
  polygon.resize(0);
  //reading table and filling the polygon
  int rows = ui->tabwidSpF->rowCount();
  for (int i=0; i<rows; i++)
    {
      double valX = ui->tabwidSpF->item(i,0)->text().toDouble();
      double valY = ui->tabwidSpF->item(i,1)->text().toDouble();
      polygon<<QPointF(valX, valY);
      //qDebug()<<valX<<valY;
    }
  polygon<<polygon.first();
}


void ReconstructionWindow::on_pbGoToNextNoise_clicked()
{
  if (EventsDataHub->isEmpty()) return;
  if (EventsDataHub->isScanEmpty()) return;

  for (int iev = ui->sbEventNumberInspect->value()+1; iev < EventsDataHub->Events.size(); iev++)
    {
      if ( !EventsDataHub->Scan[iev]->GoodEvent)
        {
          ui->sbEventNumberInspect->setValue(iev);
          return;
        }
    }
  MW->Owindow->OutText("There are no more noise events after this index!");
}

void ReconstructionWindow::on_pbGoToNextNoiseFoundGood_clicked()
{
  if (EventsDataHub->isEmpty()) return;
  if (EventsDataHub->isScanEmpty()) return;
  int CurrentGroup = PMgroups->getCurrentGroup();
  if (EventsDataHub->isReconstructionReady(CurrentGroup))
    {
      message("Run reconstruction of all events first!", this);
      return;
    }

  for (int iev = ui->sbEventNumberInspect->value()+1; iev < EventsDataHub->Events.size(); iev++)
    {
      if ( !EventsDataHub->Scan[iev]->GoodEvent)
        {
          if (EventsDataHub->ReconstructionData[CurrentGroup][iev]->GoodEvent)
            {
              ui->sbEventNumberInspect->setValue(iev);
              return;
            }
        }
    }
  MW->Owindow->OutText("There are no more noise events which pass all filters after this index!");
}

void ReconstructionWindow::on_pbStopReconstruction_toggled(bool checked)
{
  if (checked)
    {
      StopRecon = true;
      ui->pbStopReconstruction->setText("stopping...");
      emit StopRequested();
    }
  else
    {
      StopRecon = false;
      ui->pbStopReconstruction->setText("stop");
    }
}

void ReconstructionWindow::on_pbCutOffApplyToAll_clicked()
{
  double min = ui->ledFilterIndividualMin->text().toDouble();
  double max = ui->ledFilterIndividualMax->text().toDouble();
  int gr = PMgroups->getCurrentGroup();
  for (int i=0; i<MW->PMs->count(); i++)
    {
     //MW->PMs->at(i).cutOffMin = min;
     //MW->PMs->at(i).cutOffMax = max;
      PMgroups->setCutOffs(i, gr, min, max, true);
    }
  UpdateStatusAllEvents();
}

void ReconstructionWindow::on_pbReconstructTrack_clicked()
{
  ReconstructionWindow::ReconstructTrack();
}

void ReconstructionWindow::on_cbRecType_currentIndexChanged(int index)
{
    ui->swPointVsTracks->setCurrentIndex(index);
}

void ReconstructionWindow::on_pbAddToActive_clicked()
{
  ReconstructionWindow::MakeActivePassive(ui->leActivePassivePMs->text(), "active");
}

void ReconstructionWindow::on_pbAddToPassive_clicked()
{
  ReconstructionWindow::MakeActivePassive(ui->leActivePassivePMs->text(), "passive");
}

void ReconstructionWindow::MakeActivePassive(QString input, QString mode)
{
  QList<int> ToAdd;
  bool ok = ExtractNumbersFromQString(input, &ToAdd);
  if (!ok)
    {
      message("Input error!", this);
      return;
    }

  int iMaxPM = MW->PMs->count()-1;
  for (int i=0; i<ToAdd.size(); i++)
    if (ToAdd[i] >iMaxPM )
      {
        message("Range error!", this);
        return;
      }

  //no error
  if (mode == "active")
      for (int i=0; i<ToAdd.size(); i++)
          PMgroups->clearStaticPassive(ToAdd[i]);
  else if (mode == "passive")
      for (int i=0; i<ToAdd.size(); i++)
          PMgroups->setStaticPassive(ToAdd[i]);
  else
     {
      message("Unknown mode!", this);
      return;
     }

  ReconstructionWindow::on_pbShowPassivePMs_clicked();
  onUpdatePassiveIndication();
  UpdateReconConfig(); //Config->JSON update
}

void ReconstructionWindow::on_pbShowPassivePMs_clicked()
{
    MW->Detector->GeoManager->ClearTracks();
    MW->ShowGeometry();

    //cycle by PMs and if passive, add tracks for indication
    for (int ipm=0; ipm<MW->PMs->count(); ipm++)
    {
        const pm &PM = MW->PMs->at(ipm);
        //if (PM.isStaticPassive())
        if (PMgroups->isStaticPassive(ipm))
        {
            int typ = PM.type;
            double length = MW->PMs->getType(typ)->SizeX*0.25;
            Int_t track_index = MW->Detector->GeoManager->AddTrack(1,22);
            TVirtualGeoTrack *track = MW->Detector->GeoManager->GetTrack(track_index);
            track->AddPoint(PM.x-length, PM.y-length, PM.z-length, 0);
            track->AddPoint(PM.x+length, PM.y+length, PM.z+length, 0);
            track->AddPoint(PM.x, PM.y, PM.z, 0);
            track->AddPoint(PM.x-length, PM.y-length, PM.z+length, 0);
            track->AddPoint(PM.x+length, PM.y+length, PM.z-length, 0);
            track->AddPoint(PM.x, PM.y, PM.z, 0);
            track->AddPoint(PM.x-length, PM.y+length, PM.z-length, 0);
            track->AddPoint(PM.x+length, PM.y-length, PM.z+length, 0);
            track->AddPoint(PM.x, PM.y, PM.z, 0);
            track->AddPoint(PM.x-length, PM.y+length, PM.z+length, 0);
            track->AddPoint(PM.x+length, PM.y-length, PM.z-length, 0);
            track->AddPoint(PM.x, PM.y, PM.z, 0);

            track->SetLineWidth(3);
            track->SetLineColor(30);
        }
    }
    MW->ShowTracks();
}

void ReconstructionWindow::on_pbAllPMsActive_clicked()
{
  for (int ipm = 0; ipm<MW->PMs->count(); ipm++)
      //MW->PMs->at(ipm).passive = 0;
      PMgroups->clearStaticPassive(ipm);
  ReconstructionWindow::on_pbShowPassivePMs_clicked();
  UpdateReconConfig(); //Config->JSON update
  onUpdatePassiveIndication();
  MW->Owindow->RefreshData();
}

void ReconstructionWindow::on_pbMainWindow_clicked()
{
    //this->hide();
    MW->showNormal();
    MW->raise();
    MW->activateWindow();
}

void ReconstructionWindow::on_pbShowLog_clicked()
{
    MW->Owindow->SetTab(0);
    MW->Owindow->raise();
    MW->Owindow->showNormal();
    MW->Owindow->activateWindow();

    int iev = ui->sbEventNumberInspect->value();
    if (iev<EventsDataHub->Events.size()) MW->Owindow->ShowOneEventLog(iev);
}

void ReconstructionWindow::on_pbDoAutogroup_clicked()
{
  int ret = QMessageBox::warning(this, "",
                                     "This will clear all defined groups and their properties\n"
                                     "including gains, cutoffs and passive status.\n"
                                     "Groups will be created according to PM types and Z coordinates.",
                                     QMessageBox::Yes | QMessageBox::Cancel,
                                     QMessageBox::Cancel);
  if (ret == QMessageBox::Cancel) return;

  //depends on the current PM array type and whether or not GDMl was loaded
  bool RegularArray = true;
  if (!Detector->isGDMLempty()) RegularArray = false;
  else if (Detector->PMarrays.at(0).Regularity  > 1 || Detector->PMarrays.at(1).Regularity  > 1) RegularArray = false;
  else RegularArray = true;

  PMgroups->autogroup(RegularArray);

   onUpdatePMgroupIndication();
   onUpdateGainsIndication();
}

void ReconstructionWindow::on_pbClearGroups_clicked()
{
  int ret = QMessageBox::warning(this, "",
                                   "This will clear all defined groups, create DefaultGroup and assign all PMs to it.\n"
                                   "Gains, cutoffs and passive status will be reset. Continue?",
                                   QMessageBox::Yes | QMessageBox::Cancel,
                                   QMessageBox::Cancel);
  if (ret == QMessageBox::Cancel) return;

  MW->Detector->PMgroups->clearPMgroups(); //updates Config->JSON

  onUpdatePMgroupIndication();
  onUpdateGainsIndication();
}

void ReconstructionWindow::onUpdatePMgroupIndication()
{
   //qDebug() << "Updating sensor group indication. Groups:"<<PMgroups->countPMgroups();
   ReconstructionWindow::UpdatePMGroupCOBs();

   ui->lwPMgroups->clear();
   int groups = PMgroups->countPMgroups();

   QString str;
   for (int ig = 0; ig<groups; ig++)
   {
       str = QString::number(ig)+"> ";
       str += MW->Detector->PMgroups->getGroupName(ig) + "  ";

       int counter = MW->Detector->PMgroups->countPMsInGroup(ig);

       str += "( " + QString::number(counter) + " PMs)";
       ui->lwPMgroups->addItem(str);
   }
   ui->cobCurrentGroup->setCurrentIndex(PMgroups->getCurrentGroup());

   bool fUnA = ( PMgroups->countPMsWithoutGroup() != 0 );
   ui->fNotAssignedPMs->setVisible( fUnA );

   on_cobAssignToPMgroup_currentIndexChanged(ui->cobAssignToPMgroup->currentIndex());
}

void ReconstructionWindow::on_cobAssignToPMgroup_currentIndexChanged(int igroup)
{
    //showing composition
    QString vs;
    if (igroup>-1 && igroup<PMgroups->countPMgroups())
    {
        QVector<int> vi;
        for (int i=0; i<PMs->count(); i++)
            if (PMgroups->isPmBelongsToGroup(i, igroup)) vi.append(i);

        if (vi.isEmpty()) vs = "Group is empty";
        else if (vi.size()==1) vs = QString::number(vi.first());
        else
        {
            int start = vi.first();
            int current = vi.first();
            for (int i=1; i<vi.size(); i++)
            {
                if (vi.at(i) != current+1)
                {
                    //break in numbers detected
                    int from = start;
                    int to = current;
                    if (from == current)
                        vs += QString::number(from) + QString(", ");
                    else
                        vs += QString::number(from) + "-" + QString::number(to) + QString(", ");
                    start = vi.at(i);
                }
                current = vi.at(i);
            }
            vs += QString::number(start) + "-" + QString::number(vi.last());
        }
    }
    ui->leSensorGroupComposition->setText(vs);
}

void ReconstructionWindow::on_pbUpdateGainsIndication_clicked()
{  
    ReconstructionWindow::onUpdateGainsIndication();
}

void ReconstructionWindow::onUpdateGainsIndication()
{
    int CurrentGroup = PMgroups->getCurrentGroup();
    int iCurrentPM = -1;
    if (ui->leiPMnumberForGains->text() != "")
      {
        iCurrentPM = ui->leiPMnumberForGains->text().toInt();
             //if (PMs->at(icurrent).group != igroup) icurrent = -1;
          //if (!PMgroups->isPmInCurrentGroupFast(iCurrentPM)) iCurrentPM = -1; //cannot do it with confgurations with no PMs
        if (!PMgroups->isPmBelongsToGroup(iCurrentPM, CurrentGroup)) iCurrentPM = -1;
        else ui->ledGain->setText(QString::number(PMgroups->getGain(iCurrentPM, CurrentGroup)));
      }

    if (iCurrentPM == -1)
      {
        bool found = false;
        int ifound = 0;
        for (int i=0; i<MW->PMs->count(); i++)
          //if (PMs->at(i).group == igroup)
          if (PMgroups->isPmInCurrentGroupFast(i))
            {
              found = true;
              ifound = i;
              break;
            }
        if (found)
          {
            ui->leiPMnumberForGains->setText(QString::number(ifound));
            ui->ledGain->setText(QString::number(PMgroups->getGain(ifound, CurrentGroup)));
          }
    }
}

void ReconstructionWindow::on_leiPMnumberForGains_editingFinished()
{
    if (ui->leiPMnumberForGains->text() == "")
      {
        ui->ledGain->setText("");
        return;
      }

    int ipm = ui->leiPMnumberForGains->text().toInt();

    if (ipm>MW->PMs->count()-1)
      {
        ui->leiPMnumberForGains->setText("");
        message("PM with this number does not exist in this detector!", this);
        return;
      }
    if (!PMgroups->isPmInCurrentGroupFast(ipm))
      {
        ui->leiPMnumberForGains->setText("");
        message("This PM does not belong to the selected PM group!", this);
        return;
      }

    int CurrentGroup = PMgroups->getCurrentGroup();
    double gain = PMgroups->getGain(ipm, CurrentGroup);
    ui->ledGain->setText( QString::number(gain) );
}

void ReconstructionWindow::on_ledGain_editingFinished()
{
   if (ui->leiPMnumberForGains->text() == "") return;

   int ipm = ui->leiPMnumberForGains->text().toInt();
   int igroup = PMgroups->getCurrentGroup();

   if (ipm>MW->PMs->count()-1)
     {
       ui->leiPMnumberForGains->setText("");
       message("PM with this number does not exist in this detector!", this);
       return;
     }

   //if (MW->PMs->at(ipm).group != igroup)
   if (!PMgroups->isPmBelongsToGroup(ipm, igroup))
     {
       ui->leiPMnumberForGains->setText("");
       message("This PM does not belong to PM group "+QString::number(igroup), this);
       ui->leiPMnumberForGains->setText("");
       return;
     }

   double newgain = ui->ledGain->text().toDouble();
   if (newgain == 0)
     {
       double oldGain = PMgroups->getGain(ipm, igroup);
       ui->ledGain->setText(QString::number(oldGain));
       message("Gain cannot be zero!", this);
       return;
     }

   PMgroups->setGain(ipm, igroup, newgain, true);
   UpdateStatusAllEvents();
   ui->pbFindNextPMinGains->setFocus();
}

void ReconstructionWindow::on_pbShowAllGainsForGroup_clicked()
{
  int igroup = PMgroups->getCurrentGroup();
  QVector<QString> tmp(0);
  for (int i=0; i<PMs->count(); i++)
    {
      QString str;
      if (PMgroups->isPmInCurrentGroupFast(i))
        {
          double gain = PMgroups->getGain(i, igroup);
          str = QString::number( gain,'g', 4);
        }
      else str = "";
      tmp.append( str );
    }
  MW->GeometryWindow->ShowTextOnPMs(tmp, kBlue);
}

void ReconstructionWindow::on_pbGainsToUnity_clicked()
{
  int igroup = PMgroups->getCurrentGroup();
  double val = ui->ledGainsValue->text().toDouble();

  for (int ipm=0; ipm<PMs->count(); ipm++)
     PMgroups->setGain(ipm, igroup, val, true);

  ui->ledGain->setText(ui->ledGainsValue->text());
  ReconstructionWindow::on_pbShowAllGainsForGroup_clicked();
  UpdateStatusAllEvents();
}

void ReconstructionWindow::on_pbGainsMultiply_clicked()
{  
  double factor = ui->ledGainsValue->text().toDouble();
  int currentGroup = PMgroups->getCurrentGroup();

  for (int ipm=0; ipm<PMs->count(); ipm++)
  {
      double newVal = PMgroups->getGain(ipm, currentGroup) * factor;
      PMgroups->setGain(ipm, currentGroup, newVal, true);
  }

  ui->ledGain->setText( QString::number( PMgroups->getGain( ui->leiPMnumberForGains->text().toInt(), currentGroup)) );
  ReconstructionWindow::on_pbShowAllGainsForGroup_clicked();
  UpdateStatusAllEvents();
}

void ReconstructionWindow::on_pbFindNextPMinGains_clicked()
{
  int igroup = PMgroups->getCurrentGroup();
  int numPMs = MW->PMs->count();

   QString str = ui->leiPMnumberForGains->text();
   int ipmStart = numPMs-1; //to start from 0 if empty text
   if (str != "") ipmStart = str.toInt();

   int ipm = ipmStart;
   bool found = false;
   if (PMgroups->countPMsInGroup(igroup)>1)
   {
       do
         {
           ipm++;
           if (ipm > numPMs-1) ipm = 0;

           //if (MW->PMs->at(ipm).group == igroup) found = true;
           if (PMgroups->isPmBelongsToGroup(ipm, igroup)) found = true;
         }
       while (!found && ipm != ipmStart);
   }

   if (!found)
     {
       message("There are no PMs belonging to this group!", this);
       return;
     }

   ui->leiPMnumberForGains->setText( QString::number(ipm) );
   //ui->ledGain->setText( QString::number( MW->PMs->at(ipm).relGain ) );
   ui->ledGain->setText( QString::number( PMgroups->Groups.at(igroup)->PMS.at(ipm).gain ) );
}

void ReconstructionWindow::on_pbSaveGains_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save gains", MW->GlobSet->LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*.*)");
    if (fileName.isEmpty()) return;
    MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();
    QFileInfo file(fileName);
    if(file.suffix().isEmpty()) fileName += ".dat";
    QFile outputFile(fileName);
    outputFile.open(QIODevice::WriteOnly);
    if(!outputFile.isOpen())
     {
        message("Unable to open file " +fileName+ " for writing!", this);
        return;
      }
    int numPMs = PMs->count();
    int CurrentGroup = PMgroups->getCurrentGroup();
    QVector<double> X, Y;
    for (int ipm=0; ipm<numPMs; ipm++)
      {
        X.append(ipm);
        //Y.append(PMs->at(ipm).relGain);
        Y.append(PMgroups->getGain(ipm, CurrentGroup));
      }
    SaveDoubleVectorsToFile(fileName, &X, &Y);
}

void ReconstructionWindow::on_pbLoadGains_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Load gains", MW->GlobSet->LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*.*)");
//  qDebug()<<fileName;
  if (fileName.isEmpty()) return;
  MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();  
  QVector<double> num, gain;
  LoadDoubleVectorsFromFile(fileName, &num, &gain);  //cleans previous data

  int CurrentGroup = PMgroups->getCurrentGroup();
  for (int i=0; i<num.size(); i++)
    //PMs->at(i).relGain = gain[i];
    Detector->PMgroups->setGain(i, CurrentGroup, gain[i], true);

  ReconstructionWindow::on_pbShowAllGainsForGroup_clicked();
  UpdateStatusAllEvents();
  //UpdateReconConfig(); //Config->JSON update
}

void ReconstructionWindow::on_twOptions_currentChanged(int)
{
    if (!MW->PMs) return; //on start
    //ReconstructionWindow::on_pbUpdateGainsIndication_clicked();
    //ReconstructionWindow::on_pbUpdatePMgroupIndication_clicked();
}

void ReconstructionWindow::on_pbAssignPMtoGroup_clicked()
{
  QList<int> ToAdd;
  bool ok = ExtractNumbersFromQString(ui->lePMsToAssign->text(), &ToAdd);
  if (!ok)
    {
      message("Input error!", this);
      return;
    }

  int igroup = ui->cobAssignToPMgroup->currentIndex();
  for (int i=0; i<ToAdd.size(); i++)
  {
      int ipm = ToAdd[i];
      PMgroups->addPMtoGroup(ipm, igroup);
  }

  onUpdatePMgroupIndication();
  UpdateReconConfig(); //Config->JSON update
}

void ReconstructionWindow::on_pbRemovePMfromGroup_clicked()
{
    QList<int> ToRemove;
    bool ok = ExtractNumbersFromQString(ui->lePMsToAssign->text(), &ToRemove);
    if (!ok)
      {
        message("Input error!", this);
        return;
      }

    int igroup = ui->cobAssignToPMgroup->currentIndex();
    for (int i=0; i<ToRemove.size(); i++)
        PMgroups->removePMfromGroup(ToRemove[i], igroup);

    onUpdatePMgroupIndication();
    UpdateReconConfig(); //Config->JSON update
}

void ReconstructionWindow::on_pbClearThisGroup_clicked()
{
    int igroup = ui->cobAssignToPMgroup->currentIndex();
    PMgroups->removeAllPmRecords(igroup);

    onUpdatePMgroupIndication();
    UpdateReconConfig(); //Config->JSON update
}

void ReconstructionWindow::on_pbPMsAsGroup_clicked()
{
    QList<int> Group;
    bool ok = ExtractNumbersFromQString(ui->lePMsToAssign->text(), &Group);
    if (!ok)
      {
        message("Input error!", this);
        return;
      }

    int igroup = ui->cobAssignToPMgroup->currentIndex();
    PMgroups->removeAllPmRecords(igroup);
    for (int i=0; i<Group.size(); i++)
    {
        int ipm = Group[i];
        PMgroups->addPMtoGroup(ipm, igroup);
    }

    onUpdatePMgroupIndication();
    UpdateReconConfig(); //Config->JSON update
}

void ReconstructionWindow::on_pbDefineNewGroup_clicked()
{
    QString name = ui->leCustomGroupName->text();
    int groups = MW->Detector->PMgroups->countPMgroups();
    for (int i=0; i<groups; i++)
      {
        if (MW->Detector->PMgroups->getGroupName(i) == name)
          {
            message("Group with tis name was already defined!", this);
            return;
          }
      }
    MW->Detector->PMgroups->definePMgroup(name); //also updates json

    onUpdatePMgroupIndication();
    onUpdateGainsIndication();
}

void ReconstructionWindow::on_pbShowVsXY_clicked()
{  
  QString strIn = ui->cobShowWhat->currentText();
  ReconstructionWindow::ShowVsXY(strIn);
  UpdateReconConfig(); //just to update gui settings - need it if script is used in parallel with gui
}

void ReconstructionWindow::DoPlotXY(int i)
{
    if (i<0 || i > ui->cobShowWhat->count()-1) return;
    ui->cobShowWhat->setCurrentIndex(i);
    ReconstructionWindow::on_pbShowVsXY_clicked();
}

void ReconstructionWindow::on_cbPlotVsActualPosition_toggled(bool)
{
    //if (MW->GraphWindow->LastDistributionShown == "ShowVsXY") ReconstructionWindow::on_pbShowVsXY_clicked();
}

void ReconstructionWindow::on_cb3Dreconstruction_toggled(bool checked)
{
    if (checked)
      {
        ui->cbForceCoGgiveZof->setEnabled(true);
      }
    else
      {
        ui->cbForceCoGgiveZof->setEnabled(false);
        ui->cbForceCoGgiveZof->setChecked(true);
      }

    ui->fCoGStretchZ->setEnabled( ui->cb3Dreconstruction->isChecked() && !ui->cbForceCoGgiveZof->isChecked() );
    on_cobZ_currentIndexChanged(ui->cobZ->currentIndex()); //to update cob with Z strategy
}

bool ReconstructionWindow::startXextraction()
{
   MW->WindowNavigator->BusyOn();
   MW->GraphWindow->ExtractX();
   return MW->GraphWindow->Extraction();
}

bool ReconstructionWindow::start2DLineExtraction()
{
    MW->WindowNavigator->BusyOn();
    MW->GraphWindow->Extract2DLine();
    return MW->GraphWindow->Extraction();
}

bool ReconstructionWindow::start2DEllipseExtraction()
{
    MW->WindowNavigator->BusyOn();
    MW->GraphWindow->Extract2DEllipse();
    return MW->GraphWindow->Extraction();
}

void ReconstructionWindow::on_pbDefSumCutOffs_clicked()
{
   if (EventsDataHub->isEmpty()) return;
   int totNumEvents = EventsDataHub->Events.size();
   if ( totNumEvents == 0) return;

   ReconstructionWindow::on_pbShowSumSignal_clicked();

   bool ok = ReconstructionWindow::startXextraction();
   if (!ok) return;

   ui->ledFilterSumMin->setText(QString::number(MW->GraphWindow->extractedX(), 'g', 4));
   ReconstructionWindow::on_pbUpdateFilters_clicked();
   ReconstructionWindow::on_pbShowSumSignal_clicked();
}

void ReconstructionWindow::on_pbDefSumCutOffs2_clicked()
{   
   if (EventsDataHub->isEmpty()) return;
   int totNumEvents = EventsDataHub->Events.size();
   if ( totNumEvents == 0) return;

   ReconstructionWindow::on_pbShowSumSignal_clicked();

   bool ok = ReconstructionWindow::startXextraction();
   if (!ok) return;

   ui->ledFilterSumMax->setText(QString::number(MW->GraphWindow->extractedX(), 'g', 4));
   //ReconstructionWindow::on_ledFilterSumMax_editingFinished();
   ReconstructionWindow::on_pbUpdateFilters_clicked();
   ReconstructionWindow::on_pbShowSumSignal_clicked();
}

void ReconstructionWindow::on_pbDefCutOffMin_clicked()
{   
   if (EventsDataHub->isEmpty()) return;
   int totNumEvents = EventsDataHub->Events.size();
   if ( totNumEvents == 0) return;
   ReconstructionWindow::on_pbShowIndividualSpectrum_clicked();

   bool ok = ReconstructionWindow::startXextraction();
   if (!ok) return;

   ui->ledFilterIndividualMin->setText(QString::number(MW->GraphWindow->extractedX(), 'g', 4));
   ReconstructionWindow::on_ledFilterIndividualMin_editingFinished();
   ReconstructionWindow::on_pbShowIndividualSpectrum_clicked();   
}

void ReconstructionWindow::on_pbDefCutOffMax_clicked()
{   
   if (EventsDataHub->isEmpty()) return;
   int totNumEvents = EventsDataHub->Events.size();
   if ( totNumEvents == 0) return;

   ReconstructionWindow::on_pbShowIndividualSpectrum_clicked();

   bool ok = ReconstructionWindow::startXextraction();
   if (!ok) return;

   ui->ledFilterIndividualMax->setText(QString::number(MW->GraphWindow->extractedX(), 'g', 4));
   ReconstructionWindow::on_ledFilterIndividualMax_editingFinished();
   ReconstructionWindow::on_pbShowIndividualSpectrum_clicked();
}

void ReconstructionWindow::on_pbCutOffsNextPM_clicked()
{
   int ipm = ui->sbPMsignalIndividual->value();
   int i = ipm + 1;
   bool fAlreadyBeenAtZero = false;
   do
   {
       if (i == PMs->count())
       {
           i = 0;
           if (fAlreadyBeenAtZero) break;
           fAlreadyBeenAtZero = true;
       }
       if (PMgroups->isPmInCurrentGroupFast(i))
       {
           ui->sbPMsignalIndividual->setValue(i);
           return;
       }
       i++;
   }
   while (i != ipm );

   message("There are no other PMs from this sensor group", this);
}

void ReconstructionWindow::on_pbDefEnMin_clicked()
{
   int CurrentGroup = PMgroups->getCurrentGroup();
   if (!EventsDataHub->isReconstructionReady(CurrentGroup))
   {
      message("Reconstruction data are empty!", this);
      return;
   }

   ReconstructionWindow::ShowEnergySpectrum();

   bool ok = ReconstructionWindow::startXextraction();
   if (!ok) return;

   ui->ledEnergyMin->setText(QString::number(MW->GraphWindow->extractedX(), 'g', 4));
   ReconstructionWindow::on_pbUpdateFilters_clicked();
   ReconstructionWindow::ShowEnergySpectrum();
}

void ReconstructionWindow::on_pbDefEnMax_clicked()
{
   int CurrentGroup = PMgroups->getCurrentGroup();
   if (!EventsDataHub->isReconstructionReady(CurrentGroup))
   {
     message("Reconstruction data are empty!", this);
     return;
   }

   ReconstructionWindow::ShowEnergySpectrum();

   bool ok = ReconstructionWindow::startXextraction();
   if (!ok) return;

   ui->ledEnergyMax->setText(QString::number(MW->GraphWindow->extractedX(), 'g', 4));
   ReconstructionWindow::on_pbUpdateFilters_clicked();
   ReconstructionWindow::ShowEnergySpectrum();
}

void ReconstructionWindow::on_pbDefChi2Min_clicked()
{
   int CurrentGroup = PMgroups->getCurrentGroup();
   if (!EventsDataHub->isReconstructionReady(CurrentGroup))
   {
     message("Reconstruction data are empty!", this);
     return;
   }

   ReconstructionWindow::ShowChi2Spectrum();

   bool ok = ReconstructionWindow::startXextraction();
   if (!ok) return;

   ui->ledChi2Min->setText(QString::number(MW->GraphWindow->extractedX(), 'g', 4));
   ReconstructionWindow::on_pbUpdateFilters_clicked();
   ReconstructionWindow::ShowChi2Spectrum();
}

void ReconstructionWindow::on_pbDefChi2Max_clicked()
{
   int CurrentGroup = PMgroups->getCurrentGroup();
   if (!EventsDataHub->isReconstructionReady(CurrentGroup))
   {
     message("Reconstruction data are empty!", this);
     return;
   }

   ReconstructionWindow::ShowChi2Spectrum();

   bool ok = ReconstructionWindow::startXextraction();
   if (!ok) return;

   ui->ledChi2Max->setText(QString::number(MW->GraphWindow->extractedX(), 'g', 4));
   ReconstructionWindow::on_pbUpdateFilters_clicked();
   ReconstructionWindow::ShowChi2Spectrum();
}

void ReconstructionWindow::on_pbShowCorr_clicked()
{   
    //are there data?
    if (EventsDataHub->isEmpty())
     {
        message("No events!", this);
        return;
     }

    //const QString X = tmpCorrFilter->getCorrelationUnitX()->getType();
    //const QString Y = tmpCorrFilter->getCorrelationUnitY()->getType();
//    qDebug()<<"--------------------"<<X<<Y;

    int CurrentGroup = PMgroups->getCurrentGroup();

    bool GoodEventsOnly = true;
    if (ui->cbCorrIgnoreAllFilters->isChecked()) GoodEventsOnly = false;

    //with some exceptions, reconstruction data should be present
    if ( !tmpCorrFilter->getCorrelationUnitX()->isRequireReconstruction() && !tmpCorrFilter->getCorrelationUnitY()->isRequireReconstruction() )
      {
        //for these X and Y we do not need reconstruction data if all filters are ignored
        if (EventsDataHub->isReconstructionDataEmpty(CurrentGroup))
         {
           //forcing to ignore all filters - in this way we do not ask good/bad event and reconstruction is not needed
           GoodEventsOnly = false;
         }
      }
    else
      {
        if (!EventsDataHub->isReconstructionReady(CurrentGroup))
         {
           message("Reconstruction data are empty!", this);
           return;
         }
      }

/*
    //debug
    QString str="X: ";
    for (int i=0; i<tmpCorrFilter.getCorrelationUnitX()->Channels.size(); i++) str+= QString::number(tmpCorrFilter.getCorrelationUnitX()->Channels[i])+" ";
    str+="\nY: ";
    for (int i=0; i<tmpCorrFilter.getCorrelationUnitY()->Channels.size(); i++) str+= QString::number(tmpCorrFilter.getCorrelationUnitY()->Channels[i])+" ";
    qDebug()<<str;
*/

    //=====MAIN CYCLE OVER ALL EVENTS=====
    QVector<double> x;
    QVector<double> y;

      //qDebug()<<"Starting main cycle.  GoodEventsOnly="<<GoodEventsOnly;
    for (int iev=0; iev <EventsDataHub->Events.size(); iev++)
      {
        if (GoodEventsOnly)
          if (!EventsDataHub->ReconstructionData[CurrentGroup][iev]->GoodEvent) continue;

        //populating X
        x.append(tmpCorrFilter->getCorrelationUnitX()->getValue(iev));

        //populating Y
        y.append(tmpCorrFilter->getCorrelationUnitY()->getValue(iev));

      }
    //=====================================

    if (x.isEmpty())
      {
        message("There are no events which pass all the defined filters!", this);
        return;
      }

    ReconstructionWindow::showCorrelation("Correlation", tmpCorrFilter->getCorrelationUnitX()->getAxisTitle(), tmpCorrFilter->getCorrelationUnitY()->getAxisTitle(), &x, &y);
}

void ReconstructionWindow::showCorrelation(const char* histTitle, const char* xTitle, const char* yTitle, QVector<double>* x, QVector<double>* y)
{
  int xbins = ui->sbCorrBinsX->value();
  int ybins = ui->sbCorrBinsY->value();
  double xmin = 0;
  double xmax = 0;
  double ymin = 0;
  double ymax = 0;
  if (!ui->cbCorrAutoSize->isChecked())
    {
      xmin = ui->ledCorrXmin->text().toDouble();
      xmax = ui->ledCorrXmax->text().toDouble();
      ymin = ui->ledCorrYmin->text().toDouble();
      ymax = ui->ledCorrYmax->text().toDouble();
    }

  auto histCorr = new TH2D("histCorrelation", histTitle, xbins, xmin, xmax,
                                                         ybins, ymin, ymax);
  histCorr->GetXaxis()->SetTitle(xTitle);
  histCorr->GetYaxis()->SetTitle(yTitle);

  if (ui->cbCorrAutoSize->isChecked())
  {
#if ROOT_VERSION_CODE < ROOT_VERSION(6,0,0)
     histCorr->SetBit(TH2::kCanRebin);
#endif
  }

  //filling histogam
  for (int i=0; i<x->size(); i++) histCorr->Fill(x->at(i), y->at(i));

  //drawing data 
  MW->GraphWindow->Draw(histCorr, "colz");
  MW->GraphWindow->LastDistributionShown = "Correlation";

  //drawing cuts if needed
  ReconstructionWindow::DrawCuts(); 
}

void ReconstructionWindow::DrawCuts()
{
   if (!ui->cbCorrShowCut->isChecked()) return;
   if (!ui->cbCorrShowCut->isEnabled()) return;

   if (MW->GraphWindow->LastDistributionShown != "Correlation") return;

     //qDebug()<<"Drawing cuts...";
   QList<double> tmp = tmpCorrFilter->getCut()->Data;

     //qDebug()<<"CutType: "<<tmpCorrFilter->getCut()->getType();
   if (tmpCorrFilter->getCut()->getType() == "line")
         {
            //finding intersection points
              //trying left and right borders
            double point[2];
            QVector<double> PointX(0);
            QVector<double> PointY(0);

            double minX = MW->GraphWindow->getCanvasMinX();
            double maxX = MW->GraphWindow->getCanvasMaxX();
            double minY = MW->GraphWindow->getCanvasMinY();
            double maxY = MW->GraphWindow->getCanvasMaxY();

              //qDebug()<<"Reported canvas limits: "<<minX<<maxX<<minY<<maxY;

              //qDebug()<<"Data from cut:"<<tmp[0]<<tmp[1]<< tmp[2];

            bool OK  = IntersectionOfTwoLines(tmp[0], tmp[1], tmp[2], 1.0, 0, minX, point);
            if (OK)
              if (point[1]>=minY && point[1]<=maxY)
                {
                  PointX.append(point[0]);
                  PointY.append(point[1]);
                }

            OK  = IntersectionOfTwoLines(tmp[0], tmp[1], tmp[2], 0, 1.0, maxY, point);
            if (OK)
              if (point[0]>=minX && point[0]<=maxX)
                {
                  PointX.append(point[0]);
                  PointY.append(point[1]);
                }

            OK  = IntersectionOfTwoLines(tmp[0], tmp[1], tmp[2], 1.0, 0, maxX, point);
            if (OK)
              if (point[1]>=minY && point[1]<=maxY)
                {
                  PointX.append(point[0]);
                  PointY.append(point[1]);
                }

            OK  = IntersectionOfTwoLines(tmp[0], tmp[1], tmp[2], 0, 1.0, minY, point);
            if (OK)
              if (point[0]>=minX && point[0]<=maxX)
                {
                  PointX.append(point[0]);
                  PointY.append(point[1]);
                }

            ReconstructionWindow::HideCutShapes();
            CorrCutLine->SetX1(PointX[0]);
            CorrCutLine->SetY1(PointY[0]);
            CorrCutLine->SetX2(PointX[1]);
            CorrCutLine->SetY2(PointY[1]);
            CorrCutLine->SetLineColor(kRed);
            CorrCutLine->SetLineStyle(2);
            CorrCutLine->Draw();
            //MW->GraphWindow->Draw(CorrCutLine, "same");
         }

   if (tmpCorrFilter->getCut()->getType() == "ellipse")
         {
           ReconstructionWindow::HideCutShapes();
           CorrCutEllipse->SetX1(tmp[0]);
           CorrCutEllipse->SetY1(tmp[1]);
           CorrCutEllipse->SetR1(tmp[2]);
           CorrCutEllipse->SetR2(tmp[3]);
           CorrCutEllipse->SetTheta(tmp[4]);
           CorrCutEllipse->SetFillStyle(4000);
           CorrCutEllipse->SetLineColor(kRed);
           CorrCutEllipse->SetLineStyle(2);
           CorrCutEllipse->Draw();
         }

   if (tmpCorrFilter->getCut()->getType() == "polygon")
         {
//       qDebug()<<"-------------------";
//       qDebug()<<"tmpCorrFilter->DataLength:"<<tmpCorrFilter.getCut()->Data.length();
//       for (int i=0; i<tmpCorrFilter.getCut()->Data.length();i++) qDebug()<<tmpCorrFilter.getCut()->Data[i];

           ReconstructionWindow::HideCutShapes();

           CorrCutPolygon->SetPolyLine(0);
           for (int i=0; i<tmpCorrFilter->getCut()->Data.length()/2; i++)
             {
               CorrCutPolygon->SetPoint(i, tmpCorrFilter->getCut()->Data[i*2], tmpCorrFilter->getCut()->Data[i*2+1]);
//               qDebug()<<"Poly point: "<<tmpCorrFilter.getCut()->Data[i*2]<< tmpCorrFilter.getCut()->Data[i*2+1];
             }
           CorrCutPolygon->SetFillStyle(4000);
           CorrCutPolygon->SetLineColor(kRed);
           CorrCutPolygon->SetLineStyle(2);
           CorrCutPolygon->Draw();
         }

  MW->GraphWindow->UpdateRootCanvas();
}

void ReconstructionWindow::HideCutShapes()
{    
    CorrCutLine->SetLineColor(0);
    CorrCutLine->Paint();
    CorrCutEllipse->SetLineColor(0);
    CorrCutEllipse->Paint();
    CorrCutPolygon->SetLineColor(0);
    CorrCutPolygon->Paint();

    MW->GraphWindow->UpdateRootCanvas();
}

void ReconstructionWindow::on_cbCorrAutoSize_toggled(bool checked)
{
    ui->fCorrSize->setEnabled(!checked);
}

void ReconstructionWindow::on_leCorrList1_editingFinished()
{
   if (TMPignore) return;
   QList<int> tmp;
   bool ok = ExtractNumbersFromQString(ui->leCorrList1->text(), &tmp);
   if (!ok)
    {
          TMPignore = true;
          message("Input error!", this);
          ui->leCorrList1->setFocus();
          ui->leCorrList1->selectAll();
          TMPignore = false;
          return;
    }

   for (int i=0; i<tmp.size(); i++)
        if (tmp[i]<0 || tmp[i]>MW->PMs->count()-1)
          {
            TMPignore = true;
            message("PM number is out of range", this);
            ui->leCorrList1->setFocus();
            ui->leCorrList1->selectAll();
            TMPignore = false;
            return;
          }

   ReconstructionWindow::on_pbCorrUpdateTMP_clicked();
}

void ReconstructionWindow::on_leCorrList2_editingFinished()
{
  if (TMPignore) return;

  QList<int> tmp;
  bool ok = ExtractNumbersFromQString(ui->leCorrList2->text(), &tmp);
  if (!ok)
   {
         TMPignore = true;
         message("Input error!", this);
         ui->leCorrList2->setFocus();
         ui->leCorrList2->selectAll();
         TMPignore = false;
         return;
   }

  for (int i=0; i<tmp.size(); i++)
       if (tmp[i]<0 || tmp[i]>MW->PMs->count()-1)
         {
           TMPignore = true;
           message("PM number is out of range", this);
           ui->leCorrList2->setFocus();
           ui->leCorrList2->selectAll();
           TMPignore = false;
           return;
         }

   ReconstructionWindow::on_pbCorrUpdateTMP_clicked();
}

void ReconstructionWindow::on_cobCorr1_currentIndexChanged(int index)
{
  int i;
  switch (index)
    {
    case 0:
      i=1;
      break;
    case 1:
      i=0;
      break;
    case 2:
      i=2;
      break;
    default:
      i=0;
    }

  ui->stwCorr1->setCurrentIndex(i);  //0 - empty, 1 - spin, 2 -lineEdit
}

void ReconstructionWindow::on_cobCorr2_currentIndexChanged(int index)
{
  int i;
  switch (index)
    {
    case 0:
      i=1;
      break;
    case 1:
      i=0;
      break;
    case 2:
      i=2;
      break;
    default:
      i=0;
    }

  ui->stwCorr2->setCurrentIndex(i);
}

void ReconstructionWindow::on_pbCorrUpdateTMP_clicked()
{
   if (TMPignore) return;
   //qDebug()<<"Updating tmpCorrFilter from UI...";

   //ui
   ui->stwCorrCutType->setCurrentIndex(ui->cobCorrCutType->currentIndex());
   ui->pbCorrAccept->setIcon(RedIcon);

   //updating tmpCorrFilter
   CorrelationUnitGenericClass* p;
   int ThisPMgroup = PMgroups->getCurrentGroup();
   //correlation units
     //X
   if (ui->cobCorr1->currentText() == "Signal: one channel")
     {
       QList<int> Channels;
       Channels << ui->sbCorrSingle1->value();
       p = new CU_SingleChannel(Channels, EventsDataHub, PMgroups, ThisPMgroup);
     }
   else if (ui->cobCorr1->currentText() == "Signal: sum ALL channels")
     {
       QList<int> Channels;
       for (int i=0; i<PMs->count(); i++)
           Channels << i;
       p = new CU_SumAllChannels(Channels, EventsDataHub, PMgroups, ThisPMgroup);
     }
   else if (ui->cobCorr1->currentText() == "Signal: sum channels")
     {
       QList<int> Channels;
       ExtractNumbersFromQString(ui->leCorrList1->text(), &Channels);
       p = new CU_SumChannels(Channels, EventsDataHub, PMgroups, ThisPMgroup);
     }
   else if (ui->cobCorr1->currentText() == "True or loaded energy")
       p = new CU_TrueOrLoadedEnergy(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
   else if (ui->cobCorr1->currentText() == "Reconstructed energy")
       p = new CU_RecE(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
   else if (ui->cobCorr1->currentText() == "Chi2")
       p = new CU_Chi2(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
   else if (ui->cobCorr1->currentText() == "Reconstructed X")
       p = new CU_RecX(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
   else if (ui->cobCorr1->currentText() == "Reconstructed Y")
       p = new CU_RecY(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
   else if (ui->cobCorr1->currentText() == "Reconstructed Z")
       p = new CU_RecZ(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
   else return;

   tmpCorrFilter->setCorrelationUnitX(p);

     //Y
   if (ui->cobCorr2->currentText() == "Signal: one channel")
     {
       QList<int> Channels;
       Channels << ui->sbCorrSingle2->value();
       p = new CU_SingleChannel(Channels, EventsDataHub, PMgroups, ThisPMgroup);
     }
   else if (ui->cobCorr2->currentText() == "Signal: sum ALL channels")
     {
       QList<int> Channels;
       for (int i=0; i<PMs->count(); i++)
           Channels << i;
       p = new CU_SumAllChannels(Channels, EventsDataHub, PMgroups, ThisPMgroup);
     }
   else if (ui->cobCorr2->currentText() == "Signal: sum channels")
     {
       QList<int> Channels;
       ExtractNumbersFromQString(ui->leCorrList2->text(), &Channels);
       p = new CU_SumChannels(Channels, EventsDataHub, PMgroups, ThisPMgroup);
     }
   else if (ui->cobCorr2->currentText() == "True or loaded energy")
      p = new CU_TrueOrLoadedEnergy(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
   else if (ui->cobCorr2->currentText() == "Reconstructed energy")
      p = new CU_RecE(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
   else if (ui->cobCorr2->currentText() == "Chi2")
      p = new CU_Chi2(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
   else if (ui->cobCorr2->currentText() == "Reconstructed X")
      p = new CU_RecX(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
   else if (ui->cobCorr2->currentText() == "Reconstructed Y")
      p = new CU_RecY(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
   else if (ui->cobCorr2->currentText() == "Reconstructed Z")
      p = new CU_RecZ(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
   else return;

   p->EventsDataHub = MW->EventsDataHub;
   p->PMgroups = PMgroups;
   tmpCorrFilter->setCorrelationUnitY(p);

   //cut
   if (ui->cobCorrCutType->currentText() == "Line")
     {
       Cut_Line* tmp = new Cut_Line();
       tmp->Data.clear();
       tmp->Data << ui->ledCorrLineA->text().toDouble() << ui->ledCorrLineB->text().toDouble() << ui->ledCorrLineC->text().toDouble();
       tmp->CutOption = ui->cobCorrLineCutOption->currentIndex();
       tmpCorrFilter->setCut(tmp);
     }
   else if (ui->cobCorrCutType->currentText() == "Ellipse")
     {
       Cut_Ellipse* tmp = new Cut_Ellipse();
       tmp->Data.clear();
       tmp->Data << ui->ledCorrEllX->text().toDouble() << ui->ledCorrEllY->text().toDouble() << ui->ledCorrEllR1->text().toDouble() << ui->ledCorrEllR2->text().toDouble()<< ui->ledCorrEllAngle->text().toDouble();
       tmp->CutOption = ui->cobCorrEllCutOption->currentIndex();
       tmpCorrFilter->setCut(tmp);
     }
   else if (ui->cobCorrCutType->currentText() == "Polygon")
     {
       Cut_Polygon* tmp = new Cut_Polygon();
       tmp->CutOption = ui->cobCorrPolygonCutOption->currentIndex();

       int rows = ui->tabwidCorrPolygon->rowCount();
       tmp->Data.clear();
       for (int i=0; i<rows; i++)
         {
           tmp->Data << ui->tabwidCorrPolygon->item(i,0)->text().toDouble() << ui->tabwidCorrPolygon->item(i,1)->text().toDouble();
 //          qDebug()<<"adding point to poly: "<<ui->tabwidCorrPolygon->item(i,0)->text().toDouble() << ui->tabwidCorrPolygon->item(i,1)->text().toDouble();
         }

       tmp->Data << ui->tabwidCorrPolygon->item(0,0)->text().toDouble() << ui->tabwidCorrPolygon->item(0,1)->text().toDouble(); //to have closed polygon
 //      qDebug()<<"adding last point to poly: "<<ui->tabwidCorrPolygon->item(0,0)->text().toDouble() << ui->tabwidCorrPolygon->item(0,1)->text().toDouble();
 //      qDebug()<<"Data length:"<<tmp->Data.length();
       for (int i=0; i<tmp->Data.length();i++) qDebug()<<tmp->Data[i];


       tmpCorrFilter->setCut(tmp);

 //      qDebug()<<"tmpCorrFilter->DataLength:"<<tmpCorrFilter.getCut()->Data.length();
 //      for (int i=0; i<tmpCorrFilter.getCut()->Data.length();i++) qDebug()<<tmpCorrFilter.getCut()->Data[i];
     }

    ReconstructionWindow::DrawCuts();  //safe to use - it checks for all necessary conditions

    //the rest of controls
    tmpCorrFilter->BinX = ui->sbCorrBinsX->value();
    tmpCorrFilter->BinY = ui->sbCorrBinsY->value();
    tmpCorrFilter->AutoSize = ui->cbCorrAutoSize->isChecked();
    tmpCorrFilter->minX = ui->ledCorrXmin->text().toDouble();
    tmpCorrFilter->maxX = ui->ledCorrXmax->text().toDouble();
    tmpCorrFilter->minY = ui->ledCorrYmin->text().toDouble();
    tmpCorrFilter->maxY  = ui->ledCorrYmax->text().toDouble();

    //qDebug() << "...tmpCorrFilter updated";
}

void ReconstructionWindow::on_pbCorrExtractLine_clicked()
{
    //are there data?
    if (EventsDataHub->isEmpty()) return;

    if ( tmpCorrFilter->getCorrelationUnitX()->isRequireReconstruction() || tmpCorrFilter->getCorrelationUnitY()->isRequireReconstruction() )
      if (!EventsDataHub->isReconstructionReady()) return;

    ReconstructionWindow::on_pbShowCorr_clicked();
    ReconstructionWindow::HideCutShapes();

    bool ok = ReconstructionWindow::start2DLineExtraction();
    if (!ok) return;

    ui->ledCorrLineA->setText(QString::number(MW->GraphWindow->extracted2DLineA(), 'g', 4));
    ui->ledCorrLineB->setText(QString::number(MW->GraphWindow->extracted2DLineB(), 'g', 4));
    ui->ledCorrLineC->setText(QString::number(MW->GraphWindow->extracted2DLineC(), 'g', 4));
    ReconstructionWindow::on_pbCorrUpdateTMP_clicked();

    ReconstructionWindow::on_pbUpdateFilters_clicked();
    ReconstructionWindow::on_pbShowCorr_clicked();
}

void ReconstructionWindow::on_sbCorrFNumber_valueChanged(int arg1)
{
//   qDebug()<<"-------------- ===Corr filter value changed! (total filters:"<<CorrelationFilters.size()<<")";
   if (arg1<0)
     {
       if (CorrelationFilters.isEmpty())
         {
           ui->sbCorrFNumber->setValue(0);
           return;
         }
       ui->sbCorrFNumber->setValue(CorrelationFilters.size()-1);
       return;
     }

   if (arg1 == 0  && CorrelationFilters.size() == 0) return; //protection just in case

   if (arg1 > CorrelationFilters.size()-1)
     {
       ui->sbCorrFNumber->setValue(0);
       return;
     }

   //updating tmpCorrFilter
//   qDebug()<<"Updating tmpCorrFilter from CorrFilters Qvector, index="<<arg1;
   tmpCorrFilter->copyFrom(CorrelationFilters[arg1]);
   ReconstructionWindow::FillUiFromTmpCorrFilter();

   //updating "Active indication"
   ui->cbCorrActive->setChecked(CorrelationFilters[arg1]->Active);
//   qDebug()<<"active?----------"<<CorrelationFilters[arg1]->Active;

}

void ReconstructionWindow::FillUiFromTmpCorrFilter()
{
//   qDebug()<<"========== starting ui update using tmpCorrFilter object";
//   qDebug()<<"========== CorrUnit1 type:"<<tmpCorrFilter.getCorrelationUnitX()->getType();
//   qDebug()<<"========== CorrUnit2 type:"<<tmpCorrFilter.getCorrelationUnitY()->getType();
//   qDebug()<<"========== Cut type:"<<tmpCorrFilter.getCut()->getType();

   TMPignore = true; //to forbid cross-update

   ui->sbCorrBinsX->setValue(tmpCorrFilter->BinX);
   ui->sbCorrBinsY->setValue(tmpCorrFilter->BinY);
   ui->cbCorrAutoSize->setChecked(tmpCorrFilter->AutoSize);
   ui->ledCorrXmin->setText(QString::number(tmpCorrFilter->minX));
   ui->ledCorrXmax->setText(QString::number(tmpCorrFilter->maxX));
   ui->ledCorrYmin->setText(QString::number(tmpCorrFilter->minY));
   ui->ledCorrYmax->setText(QString::number(tmpCorrFilter->maxY));

   int Xindex = tmpCorrFilter->getCorrelationUnitX()->getCOBindex();
   ui->cobCorr1->setCurrentIndex(Xindex);
   int Yindex = tmpCorrFilter->getCorrelationUnitY()->getCOBindex();
   ui->cobCorr2->setCurrentIndex(Yindex);

   if (Xindex == 0) ui->sbCorrSingle1->setValue(tmpCorrFilter->getCorrelationUnitX()->Channels[0]);
   if (Yindex == 0) ui->sbCorrSingle2->setValue(tmpCorrFilter->getCorrelationUnitY()->Channels[0]);

   if (Xindex == 2)
     {       
       QString str = "";
       for (int i=0; i<tmpCorrFilter->getCorrelationUnitX()->Channels.size(); i++)
           str += QString::number(tmpCorrFilter->getCorrelationUnitX()->Channels[i])+",";
       if (str.endsWith(',')) str.chop(1);
       ui->leCorrList1->setText(str);
     }
   if (Yindex == 2)
     {
       QString str = "";
       for (int i=0; i<tmpCorrFilter->getCorrelationUnitY()->Channels.size(); i++)
           str += QString::number(tmpCorrFilter->getCorrelationUnitY()->Channels[i])+",";
       if (str.endsWith(',')) str.chop(1);
       ui->leCorrList2->setText(str);
     }

   int CutIndex = tmpCorrFilter->getCut()->getCOBindex();
   ui->cobCorrCutType->setCurrentIndex(CutIndex);

   if (CutIndex == 0)
     {
        ui->cobCorrLineCutOption->setCurrentIndex(tmpCorrFilter->getCut()->CutOption);

        ui->ledCorrLineA->setText(QString::number(tmpCorrFilter->getCut()->Data[0]));
        ui->ledCorrLineB->setText(QString::number(tmpCorrFilter->getCut()->Data[1]));
        ui->ledCorrLineC->setText(QString::number(tmpCorrFilter->getCut()->Data[2]));
     }

   if (CutIndex == 1)
     {
        ui->cobCorrEllCutOption->setCurrentIndex(tmpCorrFilter->getCut()->CutOption);

        ui->ledCorrEllX->setText(QString::number(tmpCorrFilter->getCut()->Data[0]));
        ui->ledCorrEllY->setText(QString::number(tmpCorrFilter->getCut()->Data[1]));
        ui->ledCorrEllR1->setText(QString::number(tmpCorrFilter->getCut()->Data[2]));
        ui->ledCorrEllR2->setText(QString::number(tmpCorrFilter->getCut()->Data[3]));
        ui->ledCorrEllAngle->setText(QString::number(tmpCorrFilter->getCut()->Data[4]));
     }

   if (CutIndex == 2)
     {
       ui->cobCorrPolygonCutOption->setCurrentIndex(tmpCorrFilter->getCut()->CutOption);

       ui->tabwidCorrPolygon->clearContents();
       ui->tabwidCorrPolygon->setShowGrid(false);
       int rows = tmpCorrFilter->getCut()->Data.size()/2-1; //last 2 lines: they are the first 2 lines repeated to have closed polygon

       ui->tabwidCorrPolygon->setRowCount(rows);
//       ui->tabwidCorrPolygon->setSelectionBehavior(QAbstractItemView::SelectRows);
       ui->tabwidCorrPolygon->setSelectionMode(QAbstractItemView::SingleSelection);

       for (int j=0; j<ui->tabwidCorrPolygon->rowCount(); j++)
         {
           QString str;
           str.setNum(tmpCorrFilter->getCut()->Data[j*2], 'g', 4);
           ui->tabwidCorrPolygon->setItem(j, 0, new QTableWidgetItem(str));
           str.setNum(tmpCorrFilter->getCut()->Data[j*2+1], 'g', 4);
           ui->tabwidCorrPolygon->setItem(j, 1, new QTableWidgetItem(str));
         }

       ui->tabwidCorrPolygon->resizeColumnsToContents();
       ui->tabwidCorrPolygon->resizeRowsToContents();
       //ui->tabwidCorrPolygon->setColumnWidth(0, 56);
       //ui->tabwidCorrPolygon->setColumnWidth(1, 56);
     }

   TMPignore = false;
//   qDebug()<<"========== done";
}

void ReconstructionWindow::on_pbCorrNew_clicked()
{
   CorrelationFilterStructure* NewFilter = new CorrelationFilterStructure(tmpCorrFilter);
   //NewFilter->copyFrom(&tmpCorrFilter);
//   qDebug()<<"---------------- copied";
   CorrelationFilters.append(NewFilter);  //cannot put tmpCorrFilter directly, will get problems on copy from update
//   qDebug()<<"---------------- appended!";

   ui->sbCorrFNumber->setValue(CorrelationFilters.size()-1);
//   qDebug()<<"---------------- ui updated";
   UpdateStatusAllEvents();
}

void ReconstructionWindow::on_pbCorrAccept_clicked()
{
   int i = ui->sbCorrFNumber->value();

   bool tmpFlag = CorrelationFilters[i]->Active;
   CorrelationFilters[i]->copyFrom(tmpCorrFilter);
   CorrelationFilters[i]->Active = tmpFlag;

   ui->pbCorrAccept->setIcon(QIcon());

   UpdateStatusAllEvents();
}

void ReconstructionWindow::on_cbCorrActive_toggled(bool checked)
{
    int i = ui->sbCorrFNumber->value();
    CorrelationFilters[i]->Active = checked;    
    UpdateStatusAllEvents();
}

void ReconstructionWindow::on_pbCorrRemove_clicked()
{
   int i = ui->sbCorrFNumber->value();
   if (CorrelationFilters.size() == 0) return; //cannot remove the last filter
   delete CorrelationFilters[i] ;
   CorrelationFilters.remove(i);
   if (i != 0)
     {
       i--;
       ui->sbCorrFNumber->setValue(i); //update on_change
     }
   else
     {
       ReconstructionWindow::on_sbCorrFNumber_valueChanged(0); //to update
     }
   UpdateStatusAllEvents();
}

void ReconstructionWindow::on_pbCorrExtractEllipse_clicked()
{
  //are there data?
  if (EventsDataHub->isEmpty()) return;

  if ( tmpCorrFilter->getCorrelationUnitX()->isRequireReconstruction() || tmpCorrFilter->getCorrelationUnitY()->isRequireReconstruction() )
    if (!EventsDataHub->isReconstructionReady()) return;

  ReconstructionWindow::on_pbShowCorr_clicked();
  ReconstructionWindow::HideCutShapes();

  bool ok = ReconstructionWindow::start2DEllipseExtraction();
  if (!ok) return;

  ui->ledCorrEllX->setText(QString::number(MW->GraphWindow->extracted2DEllipseX(), 'g', 4));
  ui->ledCorrEllY->setText(QString::number(MW->GraphWindow->extracted2DEllipseY(), 'g', 4));
  ui->ledCorrEllR1->setText(QString::number(MW->GraphWindow->extracted2DEllipseR1(), 'g', 4));
  ui->ledCorrEllR2->setText(QString::number(MW->GraphWindow->extracted2DEllipseR2(), 'g', 4));
  ui->ledCorrEllAngle->setText(QString::number(MW->GraphWindow->extracted2DEllipseTheta(), 'g', 4));
  ReconstructionWindow::on_pbCorrUpdateTMP_clicked();

  ReconstructionWindow::on_pbUpdateFilters_clicked();
  ReconstructionWindow::on_pbShowCorr_clicked();
}

void ReconstructionWindow::on_cbActivateCorrelationFilters_toggled(bool checked)
{   
  ui->fCorrCut->setEnabled(checked);
  ui->fCorrAddRemoe->setEnabled(checked);
  ui->cbCorrShowCut->setEnabled(checked);
  if (!checked) ui->cbCorrShowCut->setChecked(false);  
}

void ReconstructionWindow::on_pbLoadEnergySpectrum_clicked()
{
  if (!EventsDataHub->fLoadedEventsHaveEnergyInfo && EventsDataHub->isScanEmpty())
  {
      message("There are no energy data!", this);
      return;
  }

  auto hist1D = new TH1D("hist1ShLoEnSp","True/loaded energy", MW->GlobSet->BinsX, 0, 0);
  hist1D->SetXTitle("True or loaded energy");
#if ROOT_VERSION_CODE < ROOT_VERSION(6,0,0)
  hist1D->SetBit(TH1::kCanRebin);
#endif
  //check filter status only of reconstruction has been done
  bool DoFiltering = false;
  if (!EventsDataHub->isReconstructionDataEmpty()) DoFiltering = true;

//  int ienergy = MW->PMs->count(); //index of the energy channel
//  qDebug()<<"Energy channel="<<ienergy;

  for (int i=0; i < EventsDataHub->Events.size(); i++)
    {
       if (DoFiltering)
         if (!EventsDataHub->ReconstructionData[0][i]->GoodEvent) continue;

       //hist1D->Fill(EventsDataHub->Events[i][ienergy]);
       hist1D->Fill(EventsDataHub->Scan.at(i)->Points.at(0).energy);
    }

  MW->GraphWindow->Draw(hist1D);
  //showing range
  double minY = 0;
  double maxY = MW->GraphWindow->getCanvasMaxY();
  double filtMin = ui->ledLoadEnergyMin->text().toDouble();
  double filtMax = ui->ledLoadEnergyMax->text().toDouble();
  TLine* L1 = new TLine(filtMin, minY, filtMin, maxY);
  L1->SetLineColor(kGreen);
  L1->Draw();
  MW->GraphWindow->RegisterTObject(L1);
  TLine* L2 = new TLine(filtMax, minY, filtMax, maxY);
  L2->SetLineColor(kGreen);
  L2->Draw();
  MW->GraphWindow->RegisterTObject(L2);

  MW->GraphWindow->UpdateRootCanvas();
  MW->GraphWindow->LastDistributionShown = "LoadedEnergySpectrum";
}

void ReconstructionWindow::on_pbDefLoadEnMin_clicked()
{
  if (EventsDataHub->Events.isEmpty())
  {
     message("No data are loaded!", this);
     return;
  }

  ReconstructionWindow::on_pbLoadEnergySpectrum_clicked();

  bool ok = ReconstructionWindow::startXextraction();
  if (!ok) return;

  ui->ledLoadEnergyMin->setText(QString::number(MW->GraphWindow->extractedX(), 'g', 4));
  ReconstructionWindow::on_pbUpdateFilters_clicked();
  ReconstructionWindow::on_pbLoadEnergySpectrum_clicked();
}

void ReconstructionWindow::on_pbDefLoadEnMax_clicked()
{
  if (EventsDataHub->Events.isEmpty())
  {
     message("No data are loaded!", this);
     return;
  }

  ReconstructionWindow::on_pbLoadEnergySpectrum_clicked();

  bool ok = ReconstructionWindow::startXextraction();
  if (!ok) return;

  ui->ledLoadEnergyMax->setText(QString::number(MW->GraphWindow->extractedX(), 'g', 4));
  ReconstructionWindow::on_pbUpdateFilters_clicked();
  ReconstructionWindow::on_pbLoadEnergySpectrum_clicked();
}

void ReconstructionWindow::on_pbShowCorrShowRangeOnGraph_clicked()
{
  if (EventsDataHub->isEmpty()) return;

  if ( tmpCorrFilter->getCorrelationUnitX()->isRequireReconstruction() || tmpCorrFilter->getCorrelationUnitY()->isRequireReconstruction() )
    if (!EventsDataHub->isReconstructionReady()) return;

  ReconstructionWindow::on_pbShowCorr_clicked();
  ReconstructionWindow::HideCutShapes();

  bool ok = ReconstructionWindow::start2DBoxExtraction();
  if (!ok) return;

  ui->ledCorrXmin->setText(QString::number(MW->GraphWindow->extractedX1(), 'g', 4));
  ui->ledCorrXmax->setText(QString::number(MW->GraphWindow->extractedX2(), 'g', 4));
  ui->ledCorrYmin->setText(QString::number(MW->GraphWindow->extractedY1(), 'g', 4));
  ui->ledCorrYmax->setText(QString::number(MW->GraphWindow->extractedY2(), 'g', 4));

  TMPignore=true;
  ui->cbCorrAutoSize->setChecked(false);
  TMPignore=false;

  ReconstructionWindow::on_pbCorrUpdateTMP_clicked();

  ReconstructionWindow::on_pbShowCorr_clicked();
}

bool ReconstructionWindow::start2DBoxExtraction()
{
    MW->WindowNavigator->BusyOn();
    MW->GraphWindow->Extract2DBox();

    return MW->GraphWindow->Extraction();
}

void ReconstructionWindow::on_pbCorr_AddLine_clicked()
{
  //add line before
    int selected = ui->tabwidCorrPolygon->currentRow();
      //qDebug()<<selected;

    tmpCorrFilter->getCut()->Data.insert(selected*2, 0);
    tmpCorrFilter->getCut()->Data.insert(selected*2, 0);
    if (selected == 0)
      {
        //have to replace the last node (hidden) too
        tmpCorrFilter->getCut()->Data.replace(tmpCorrFilter->getCut()->Data.size()-1, 0);
        tmpCorrFilter->getCut()->Data.replace(tmpCorrFilter->getCut()->Data.size()-2, 0);
      }
    ReconstructionWindow::on_pbCorrUpdateTMP_clicked();
}

void ReconstructionWindow::on_ppCorr_AddLineAfter_clicked()
{
  //add line after
   int selected = ui->tabwidCorrPolygon->currentRow();
     //qDebug()<<selected;

   tmpCorrFilter->getCut()->Data.insert(selected*2, 0);
   tmpCorrFilter->getCut()->Data.insert(selected*2, 0);

   ReconstructionWindow::on_pbCorrUpdateTMP_clicked();
}

void ReconstructionWindow::on_pbCorr_RemoveLine_clicked()
{
   int selected = ui->tabwidCorrPolygon->currentRow();
//    qDebug()<<selected;
   if (selected == -1)
    {
      message("select a row first!", this);
      return;
    }
  if (ui->tabwidCorrPolygon->rowCount()<4)
    {
      message("Minimum is three rows!", this);
      return;
    }

  tmpCorrFilter->getCut()->Data.removeAt(selected*2);
  tmpCorrFilter->getCut()->Data.removeAt(selected*2);

  if (selected == 0)
    {
      //have to change the last point of polygon too (it is not shown in the table)
      tmpCorrFilter->getCut()->Data[tmpCorrFilter->getCut()->Data.size()-2] = tmpCorrFilter->getCut()->Data[0];
      tmpCorrFilter->getCut()->Data[tmpCorrFilter->getCut()->Data.size()-1] = tmpCorrFilter->getCut()->Data[1];
    }
  ReconstructionWindow::on_pbCorrUpdateTMP_clicked();
}

void ReconstructionWindow::on_tabwidCorrPolygon_cellChanged(int row, int column)
{
  if (TMPignore) return; //do not update during filling with data!

  bool ok;
  QString str = ui->tabwidCorrPolygon->item(row,column)->text();
  //double newVal =
  str.toDouble(&ok);
    //qDebug()<<str<<newVal;
  if (!ok)
    {
      message("Input error!", this);
      ReconstructionWindow::FillUiFromTmpCorrFilter();
      return;
    }
  else ReconstructionWindow::on_pbCorrUpdateTMP_clicked();
}

void ReconstructionWindow::on_pbCorrExtractPolygon_clicked()
{
  if (EventsDataHub->isEmpty()) return;

  if ( tmpCorrFilter->getCorrelationUnitX()->isRequireReconstruction() || tmpCorrFilter->getCorrelationUnitY()->isRequireReconstruction() )
    if (!EventsDataHub->isReconstructionReady()) return;

  ReconstructionWindow::on_pbShowCorr_clicked();
  ReconstructionWindow::HideCutShapes();

  bool ok = ReconstructionWindow::start2DPolygonExtraction();
  if (!ok) return;

  //get data from graph and put it to tmpCorrFilter
 // for (int i=0; i<MW->graphRW->extractedPolygon.size()/2; i++)
//    qDebug()<<"----------------polygon point: "<<MW->graphRW->extractedPolygon[i]<<MW->graphRW->extractedPolygon[i+1];
  tmpCorrFilter->getCut()->Data = MW->GraphWindow->extractedPolygon();
  ui->pbCorrAccept->setIcon(RedIcon); //sice tmpCorrFilter changed!

  //update UI
  ReconstructionWindow::FillUiFromTmpCorrFilter();

  ReconstructionWindow::on_pbUpdateFilters_clicked();
  ReconstructionWindow::on_pbShowCorr_clicked();
}

bool ReconstructionWindow::start2DPolygonExtraction()
{
    MW->WindowNavigator->BusyOn();
    MW->GraphWindow->Extract2DPolygon();

    return MW->GraphWindow->Extraction();
}

void ReconstructionWindow::on_pbEvaluateGains_clicked()
{
    ReconstructionWindow::ShowGainWindow();
}

void ReconstructionWindow::ShowGainWindow()
{
  if (!MW->GainWindow)
    {
      //first time accessed, creating
      //qDebug()<<"-->Creating GainWindow";
      QWidget* w = new QWidget();
      MW->GainWindow = new GainEvaluatorWindowClass(w, MW, EventsDataHub);
      //MW->GainWindow = new GainEvaluatorWindowClass(this, MW, EventsDataHub);

      // fix font size on Linux
      #ifdef Q_OS_WIN//Q_OS_LINUX
      #else
        SetWindowFont(MW->GainWindow, 8);
  //      GainWindow->SetWindowFont(8);
      #endif
      MW->GainWindow->show();
      MW->GainWindow->resize(MW->GainWindow->width()+1, MW->GainWindow->height());
      MW->GainWindow->resize(MW->GainWindow->width()-1, MW->GainWindow->height());

      connect(PMgroups, SIGNAL(CurrentSensorGroupChanged()), MW->GainWindow, SLOT(onCurrentSensorGroupsChanged()));
    }
  MW->GainWindow->show();
  MW->GainWindow->raise();
  MW->GainWindow->activateWindow();  
}

void ReconstructionWindow::on_pbSetPresprocessingForGains_clicked()
{
  QVector<double> Gains;
  for (int ipm=0; ipm<PMs->count(); ipm++)
  {
      double gain = 1.0;
      bool fFound = false;
      for (int igroup=0; igroup<PMgroups->countPMgroups(); igroup++)
      {
          if (PMgroups->isPmBelongsToGroup(ipm, igroup))
          {
              if (!fFound)
              {
                  fFound = true;
                  gain = PMgroups->getGain(ipm, igroup);
              }
              else
              {
                  if (gain != PMgroups->getGain(ipm, igroup))
                  {
                      QString s = "Cannot perform correction: PM# ";
                      s += QString::number(ipm) + " has different gains in different groups";
                      message(s, this);
                      return;
                  }
              }
          }
      }
      Gains << gain;
  }

  MW->SetMultipliersUsingGains(Gains);

  PMgroups->setAllGainsToUnity(); //also does JSON update
  ui->ledGain->setText("1");
}

void ReconstructionWindow::on_pbGotoPreprocessAdjust_clicked()
{
  if (MW->TmpHub->ChPerPhEl_Sigma2.size() != PMs->count())
  {
      message("Data not yet ready", this);
      return;
  }
  MW->SetMultipliersUsingChPhEl(MW->TmpHub->ChPerPhEl_Sigma2);
}

void ReconstructionWindow::on_pbPurgeEvents_clicked()
{
    if (EventsDataHub->isEmpty()) return;
    int counter = 0;
    for (int i=0; i<EventsDataHub->ReconstructionData[0].size(); i++)
        if (!EventsDataHub->ReconstructionData.at(0).at(i)->GoodEvent) counter++;
    if (counter == 0)
      {
        message("There are no filtered events!", this);
        return;
      }    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Purge events confirmation");
    msgBox.setText("Purge " + QString::number(counter) + " events?\nSelection: Any event filter or reconstruction fail\nThere is no undo!");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    if (msgBox.exec() == QMessageBox::No) return;

    MW->WindowNavigator->BusyOn();
    qApp->processEvents();
    EventsDataHub->PurgeFilteredEvents(); // ***!!! is kNN filter allright after this?
    MW->WindowNavigator->BusyOff(false);
}

void ReconstructionWindow::on_pbPurgeFraction_clicked()
{
  if (EventsDataHub->isEmpty()) return;
  int LeaveOnePer = ui->sbLeaveOneFrom->value();

  QMessageBox msgBox(this);
  msgBox.setWindowTitle("Purge events confirmation");
  msgBox.setText("Purge events leaving one per " +QString::number(LeaveOnePer)+"events?\nThere is no undo!");
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox.setDefaultButton(QMessageBox::Yes);
  if (msgBox.exec() == QMessageBox::No) return;

  MW->WindowNavigator->BusyOn();
  qApp->processEvents();
  EventsDataHub->Purge(LeaveOnePer); // ***!!! is kNN filter allright after this?
  MW->WindowNavigator->BusyOff(false);
}

void ReconstructionWindow::on_cbCoGapplyStretch_toggled(bool checked)
{
  ui->fCoGStretchXY->setEnabled(checked);
  ui->fCoGStretchZ->setEnabled(checked && ui->cb3Dreconstruction->isChecked());
}

void ReconstructionWindow::on_pbTreeView_clicked()
{   
  if (EventsDataHub->isEmpty())
    {
      message("Data are empty!", this);
      ui->twData->setCurrentIndex(0);
      return;
    }

  int igroup = PMgroups->getCurrentGroup();
  QString what = ui->cobTreeViewWhat->currentText();
  QString cond = ui->cobTreeViewCuts->currentText();
  QString how = ui->cobTreeViewOptions->currentText();
  if (what == "")
    {
      message("What to draw?", this);
      return;
    }
  QStringList fields = what.split(":", QString::SkipEmptyParts);
  int num = fields.size();
  if (num>3)
    {
      message("Too many fields in \"What\"", this);
      return;
    }

  QSet<QString> set;
  set << "x"<<"y"<<"z"<<"energy"<<"rho"<<"chi2"<<"recOK"<<"good";
  for (auto s : fields)
  {
      if (set.contains(s))
      {
          if (!EventsDataHub->isReconstructionReady(igroup))
            {
              if (igroup>0) message("Reconstruction was not yet performed for this group!", this);
              else message("Reconstruction was not yet performed!", this);
              return;
            }
      }
  }

  if (!EventsDataHub->ReconstructionTree)
    {      
      MW->WindowNavigator->BusyOn();
      bool fOK = EventsDataHub->createReconstructionTree(PMs, true, true, true, igroup);
      MW->WindowNavigator->BusyOff(false);
      if (!fOK)
      {
          message("Failed to build reconstruction data tree", this);
          return;
      }
    }
  EventsDataHub->ReconstructionTree->ResetBranchAddresses(); //if addresses are not resetted std::vectors cause crash on attempt to draw

  QByteArray baw;
  QVector<linkCustomClass> dims;
  dims.append(linkCustomClass(ui->sbBins1, ui->ledCustomFrom1, ui->ledCustomTo1));
  dims.append(linkCustomClass(ui->sbBins2, ui->ledCustomFrom2, ui->ledCustomTo2));
  dims.append(linkCustomClass(ui->sbBins3, ui->ledCustomFrom3, ui->ledCustomTo3));
  QString str = what+">>htemp(";
  for (int i=0; i<num; i++)
    {
      if (ui->cbCustomBinning->isChecked())dims[i].updateBins();
      if (ui->cbCustomRanges->isChecked()) dims[i].updateRanges(); //else 0
      str += QString::number(dims[i].bins)+","+QString::number(dims[i].from)+","+QString::number(dims[i].to)+",";
    }
  str.chop(1);
  str += ")";
  baw = str.toLocal8Bit();

  const char *What = baw.data();
   QByteArray bac = cond.toLocal8Bit();
   const char *Cond = (cond == "") ? "" : bac.data();
   QByteArray bah = how.toLocal8Bit();
   const char *How = (how == "") ? "" : bah.data();
   QString howAdj;
   if (how == "") howAdj = "goff";
   else howAdj = "goff,"+how;
   QByteArray baha = howAdj.toLocal8Bit();
   const char *HowAdj = baha.data();

   TObject* oldObj = gDirectory->FindObject("htemp");
   if (oldObj)
   {
       qDebug() << "Old htemp found!"<<oldObj->GetName();
       gDirectory->RecursiveRemove(oldObj);
   }
   TH1::AddDirectory(true);
   EventsDataHub->ReconstructionTree->Draw(What, Cond, HowAdj);
   TH1::AddDirectory(false);

   switch (num)
     {
       case 1:
       {         
         TH1* tmpHist1D = (TH1*)gDirectory->Get("htemp");
         if (!tmpHist1D)
           {
             message("Root has not generated any data!", this);
             return;
           }
         QByteArray tmp = fields[0].toLocal8Bit();
         tmpHist1D->GetXaxis()->SetTitle(tmp.data());
         int size = tmpHist1D->GetEntries();
         if (size>0) MW->GraphWindow->Draw(tmpHist1D, How);
         else
           {
             message("There is no data to show!", this);
             MW->GraphWindow->close();
             delete tmpHist1D;
             return;
           }
         break;
       }
     case 2:
       {         
         TH2* tmpHist2D = (TH2*)gDirectory->Get("htemp");
         if (!tmpHist2D)
           {
             message("Root has not generated any data!", this);
             return;
           }
         QByteArray tmp1 = fields[0].toLocal8Bit();
         tmpHist2D->GetYaxis()->SetTitle(tmp1.data());
         QByteArray tmp2 = fields[1].toLocal8Bit();
         tmpHist2D->GetXaxis()->SetTitle(tmp2.data());

         int size = tmpHist2D->GetEntries();
         if (size>0) MW->GraphWindow->Draw(tmpHist2D, How);
         else
           {
             message("There is no data to show!", this);
             MW->GraphWindow->close();
             delete tmpHist2D;
             return;
           }
         break;
       }
     case 3:
       {         
         TH3* tmpHist3D = (TH3*)gDirectory->Get("htemp");
         if (!tmpHist3D)
           {
             message("Root has not generated any data!", this);
             return;
           }
         QByteArray tmp1 = fields[0].toLocal8Bit();
         tmpHist3D->GetZaxis()->SetTitle(tmp1.data());
         QByteArray tmp2 = fields[1].toLocal8Bit();
         tmpHist3D->GetYaxis()->SetTitle(tmp2.data());
         QByteArray tmp3 = fields[2].toLocal8Bit();
         tmpHist3D->GetXaxis()->SetTitle(tmp3.data());

         int size = tmpHist3D->GetEntries();
         if (size>0) MW->GraphWindow->Draw(tmpHist3D, How);
         else
           {
             message("There is no data to show!", this);
             MW->GraphWindow->close();
             delete tmpHist3D;
             return;
           }
         break;
       }
     }

   //Updating history
   TreeViewStruct tv = TreeViewStruct(what, cond, how);
   tv.fillCustom(ui->cbCustomBinning->isChecked(), ui->cbCustomRanges->isChecked(), dims);
   TreeViewHistory.append( tv );
   TMPignore = true;
   ui->sbTreeViewHistory->setValue( TreeViewHistory.size()-1 );
   TMPignore = false;
   ui->cobTreeViewWhat->insertItem(0, what);
   for (int i = ui->cobTreeViewWhat->count()-1; i>0; i--)
       if (ui->cobTreeViewWhat->itemText(i) == what) ui->cobTreeViewWhat->removeItem(i);
   ui->cobTreeViewWhat->setCurrentIndex(0);

   ui->cobTreeViewCuts->insertItem(0, cond);
   for (int i=ui->cobTreeViewCuts->count()-1; i>0; i--)
       if (ui->cobTreeViewCuts->itemText(i) == cond) ui->cobTreeViewCuts->removeItem(i);
   ui->cobTreeViewCuts->setCurrentIndex(0);

   ui->cobTreeViewOptions->insertItem(0, how);
   for (int i=ui->cobTreeViewOptions->count()-1; i>0; i--)
       if (ui->cobTreeViewOptions->itemText(i) == how) ui->cobTreeViewOptions->removeItem(i);
   ui->cobTreeViewOptions->setCurrentIndex(0);
}

void ReconstructionWindow::on_pbTreeViewHelpWhat_clicked()
{
   QString str;

   str = "\tValid entries:\n\n "

         "\t Event data\n"
         " i\t\t event number\n"
         " signal[ipm]\t signal of ipm's PM\n"
         " ssum \t\t sum signal of all PMS (weighted by gains) \n"

         "\t Reconstruction results\n"
         "\t (for double event use [index] to select one point\n"
         " x[], y[], z[]\t reconstructed coordinates\n"
         " energy[] \t reconstruted energy\n"
         " rho[ipm] \t distance from xyz to the center of ipm's PM\n"
         " chi2 \t\t chi2 of the reconstruction\n"
         " recOK \t\t reconstruction event successful = true \n"
         " good \t\t true is recOK and all filters true\n"

         " \t Event filters\n"
         " fcut \t\t cut-off for individual signals for each PM \n"
         " fsumcut \t sum signal \n"
         " fen \t\t event energy \n"
         " fchi \t\t reduced chi-square \n"
         " fsp \t\t spatial filter\n"

         " \tTrue data (only for simulations or loaded calibration data)\n"
         " xScan[] \ttrue coordinates \n"
         " yScan[]\n"
         " zScan[]\n"
         " zStop     \t For secondary scint, light is emitted in range zScan->zStop\n"
         " numPhotons[] number of photons this event \n"
         " ScintType  \t 0 - unknown, 1 - primary, 2 - secondary \n"
         " GoodEvent  \t false for noise events \n"
         " EventType  \t type of noise event \n"
/*
              "\n\n"
              "   The item to be plotted can contain up to three entries, one for each dimension, <br>"
              "   separated by a colon (e1:e2:e3), where e1, e2 and e3 are valid entries. <br>"
              "   Each configuration in this field is under the conditions defined by 'Cuts' and  <br>"
              "   'Options' configurations. <br>"
              "    \n"
*/
              "\n\tExamples:\n"
              "          x - This entry will draw an histogram of all x\n"
              "        x:y - This will draw x vs. y in a 2D plot. \n"
              "      x:y:z - To draw x vs. y vs. z in a 3D plot. ";

   MW->Owindow->OutText(str);
   MW->Owindow->SetTab(0);
   message(str, this);
}

void ReconstructionWindow::on_pbTreeViewHelpCuts_clicked()
{
    QString str;

    str = "Any combination of logicaloperation on the valid entries (see help for 'Draw what')\n"
          "For example: energy>1.2 && chi2<2.1";

    message(str, this);
}

void ReconstructionWindow::on_pbTreeViewHelpOptions_clicked()
{
    QString str;

    str = "See CERN ROOT help for histogram drawing options.\n"
          "For example: 1D hist: \"APL\", 2D hist: \"lego\"";

    message(str, this);
}

void ReconstructionWindow::on_sbTreeViewHistory_valueChanged(int arg1)
{
  if (TMPignore) return;
  int maxEvN = TreeViewHistory.size();
  if (arg1 >= maxEvN)
  {
      ui->sbTreeViewHistory->setValue(maxEvN-1);
      return; //update is on_change
  }

  TreeViewStruct tv = TreeViewHistory[arg1];
  ui->cobTreeViewWhat->setCurrentText(tv.what);
  ui->cobTreeViewCuts->setCurrentText(tv.cuts);
  ui->cobTreeViewOptions->setCurrentText(tv.options);
  ui->cbCustomBinning->setChecked(tv.fCustomBins);
  ui->cbCustomRanges->setChecked(tv.fCustomRanges);
  if (tv.fCustomBins)
    {
      ui->sbBins1->setValue(tv.Bins[0]);
      ui->sbBins2->setValue(tv.Bins[1]);
      ui->sbBins3->setValue(tv.Bins[2]);
    }
  if (tv.fCustomRanges)
    {
      ui->ledCustomFrom1->setText(QString::number(tv.From[0]));
      ui->ledCustomFrom2->setText(QString::number(tv.From[1]));
      ui->ledCustomFrom3->setText(QString::number(tv.From[2]));
      ui->ledCustomTo1->setText(QString::number(tv.To[0]));
      ui->ledCustomTo2->setText(QString::number(tv.To[1]));
      ui->ledCustomTo3->setText(QString::number(tv.To[2]));
    }
}

void ReconstructionWindow::on_cobCUDAoffsetOption_currentIndexChanged(int index)
{
   ui->fCUDAoffsets->setVisible(index == 2);
}

struct ZSortStruct
{
  int iev;
  int pmOverThreshold;
};

bool lessThanSorter(const ZSortStruct* s1, const ZSortStruct* s2)
 {
     return s1->pmOverThreshold < s2->pmOverThreshold;
 }

void ReconstructionWindow::on_pbEvaluateZfromDistribution_clicked()
{
   if (ui->cobLRFmodule->currentIndex() != 0)
     {
       message("Not implemented yet for the new LRF module!", this);
       return;
     }

   SensorLRFs* SensLRF = Detector->LRFs->getOldModule();
   if (EventsDataHub->isReconstructionDataEmpty()) return;
   TString type = (*SensLRF)[0]->type();
   int currentGr = PMgroups->getCurrentGroup();
   if ( type != "Sliced3D")
     {
       message("This algorithm works only with Sliced3D LRFs", this);
       return;
     }

   MW->WindowNavigator->BusyOn();
   qApp->processEvents();

   QList<ZSortStruct*> sorter;
   sorter.reserve(EventsDataHub->Events.size());

   for (int iev=0; iev < EventsDataHub->Events.size(); iev++)
     {
       ZSortStruct* tmp = new ZSortStruct();
       tmp->iev = iev;

       double ssig = 0;
       double maxsig = 0;
       for (int ipm=0; ipm<MW->PMs->count(); ipm++)
        if (PMgroups->isPmInCurrentGroupFast(ipm))
         {
           //double sig = EventsDataHub->Events.at(iev)[ipm] / MW->PMs->at(ipm).relGain;
           double sig = EventsDataHub->Events.at(iev)[ipm] / PMgroups->Groups.at(currentGr)->PMS.at(ipm).gain;
           ssig += sig;
           if (sig > maxsig) maxsig = sig;
         }
       double avSig = ssig / PMs->count();

       int AboveThr = 0;
       double fraction = ui->ledZevaluatorThreshold->text().toDouble();
       for (int ipm=0; ipm<PMs->count(); ipm++)
        if (PMgroups->isPmInCurrentGroupFast(ipm))
         {
           //double sig = EventsDataHub->Events.at(iev)[ipm] / MW->PMs->at(ipm).relGain;
           double sig = EventsDataHub->Events.at(iev)[ipm] / PMgroups->Groups.at(currentGr)->PMS.at(ipm).gain;
           double threshold = avSig + fraction*(maxsig - avSig);
           if (sig > threshold) AboveThr++;
         }
       tmp->pmOverThreshold = AboveThr;
       sorter.append(tmp);
     }

   //sorting
   qSort(sorter.begin(), sorter.end(), lessThanSorter);

//   for (int i=0; i<Reconstructor->Events->size(); i++)
//     qDebug()<<sorter[i]->iev<<sorter[i]->pmOverThreshold<<"    z="<<Reconstructor->Scan->at(sorter[i]->iev)->Points[0].r[2];

   //slices?
   LRFsliced3D *lrf = (LRFsliced3D*)(*SensLRF)[0];
   int slices = lrf->getNintZ();

   int evPerSlice = EventsDataHub->Events.size()/slices;
   for (int i=0; i < EventsDataHub->Events.size(); i++)
     {
       int iz = slices-1 - i / evPerSlice;
       double z = lrf->getSliceMedianZ(iz);

       EventsDataHub->ReconstructionData[currentGr][sorter[i]->iev]->Points[0].r[2] = z;
     }

   MW->WindowNavigator->BusyOff();
}

void ReconstructionWindow::on_pbBlurReconstructedXY_clicked()
{
  MW->WindowNavigator->BusyOn();

  bool fApplyToAllGroup = ui->cbOnBlurApplyToAllGroups->isChecked();
  int igroup = (fApplyToAllGroup ? -1 : PMgroups->getCurrentGroup());

  int type = ui->cobBlurType->currentIndex();
  if (type == 0)
    { //uniform
      double delta = ui->ledBlurDelta->text().toDouble();
      EventsDataHub->BlurReconstructionData(0, delta, MW->Detector->RandGen, igroup);
    }
  else
    { //Gauss
      double sigma = ui->ledBlurSigma->text().toDouble();
      EventsDataHub->BlurReconstructionData(1, sigma, MW->Detector->RandGen, igroup);
    }

   if (ui->cbOnBlurUpdateFilters->isChecked()) UpdateStatusAllEvents();
   MW->WindowNavigator->BusyOff(false);
}

void ReconstructionWindow::on_pbBlurReconstructedZ_clicked()
{
  MW->WindowNavigator->BusyOn();

  bool fApplyToAllGroup = ui->cbOnBlurApplyToAllGroups->isChecked();
  int igroup = (fApplyToAllGroup ? -1 : PMgroups->getCurrentGroup());

  int type = ui->cobBlurType->currentIndex();
  if (type == 0)
    { //uniform
      double delta = ui->ledBlurDeltaZ->text().toDouble();
      EventsDataHub->BlurReconstructionDataZ(0, delta, MW->Detector->RandGen, igroup);
    }
  else
    { //Gauss
      double sigma = ui->ledBlurSigmaZ->text().toDouble();
      EventsDataHub->BlurReconstructionDataZ(1, sigma, MW->Detector->RandGen, igroup);
    }

   if (ui->cbOnBlurUpdateFilters->isChecked()) UpdateStatusAllEvents();
   MW->WindowNavigator->BusyOff(false);
}

void ReconstructionWindow::on_pbDefNNMin_clicked()
{
#ifdef ANTS_FLANN 
  if (EventsDataHub->isEmpty())
  {
    message("There are no events!", this);
    return;
  }
  ReconstructionWindow::on_pbNNshowSpectrum_clicked();

  bool ok = ReconstructionWindow::startXextraction();
  if (!ok) return;
  ui->ledNNmin->setText(QString::number(MW->GraphWindow->extractedX(), 'g', 4));
  ReconstructionWindow::UpdateStatusAllEvents();
  ReconstructionWindow::on_pbNNshowSpectrum_clicked();
  ReconstructionWindow::on_pbUpdateReconConfig_clicked(); //Update Config->JSON
#endif
}

void ReconstructionWindow::on_pbDefNNMax_clicked()
{
#ifdef ANTS_FLANN  
  if (EventsDataHub->isEmpty())
  {
    message("There are no events!", this);
    return;
  }
  ReconstructionWindow::on_pbNNshowSpectrum_clicked();

  bool ok = ReconstructionWindow::startXextraction();
  if (!ok) return;
  ui->ledNNmax->setText(QString::number(MW->GraphWindow->extractedX(), 'g', 4));
  ReconstructionWindow::UpdateStatusAllEvents();
  ReconstructionWindow::on_pbNNshowSpectrum_clicked();
  ReconstructionWindow::on_pbUpdateReconConfig_clicked(); //Update Config->JSON
#endif
}

void ReconstructionWindow::on_pbShowMap_clicked()
{
  int iev = ui->sbEventNumberInspect->value();
  int CurrentGroup = PMgroups->getCurrentGroup();

  if (EventsDataHub->isEmpty()) return;
  if (!EventsDataHub->isReconstructionReady(CurrentGroup))
    {
      message("Run events reconstruction first", this);
      return;
    }

  if (!Detector->LRFs->isAllLRFsDefined())
    {
      message("All LRFs have to be defined!", this);
      return;
    }  

  int MapCenterOption = ui->cobMapCenter->currentIndex();
  if (MapCenterOption == 1 && EventsDataHub->isScanEmpty())
    {
      message("Scan does not exists: changing center option to 'CoG'", this);
      ui->cobMapCenter->setCurrentIndex(0);
      MapCenterOption = 0;
    }
  if (MapCenterOption == 2 && !EventsDataHub->isReconstructionReady(CurrentGroup))
    {
      message("To center on reconstructed position, reconstruction of all events has to be performed first!", this);
      return;
    }
  int ZOption = ui->cobMapZ->currentIndex();
  if (ZOption == 1 && !EventsDataHub->isReconstructionReady(CurrentGroup))
    {
      message("To get reconstructed Z, reconstruction of all events has to be performed first!", this);
      return;
    }
  double z;
  if (ZOption == 0) z = ui->ledMapFixedZ->text().toDouble();
  else z = EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[2];

  DynamicPassivesHandler *Passives = new DynamicPassivesHandler(MW->Detector->PMs, PMgroups, EventsDataHub);
  if (CurrentGroup > EventsDataHub->RecSettings.size()-1)
      Passives->init(0, CurrentGroup);
  else
  {
      Passives->init(&EventsDataHub->RecSettings[CurrentGroup], CurrentGroup);
      if (EventsDataHub->RecSettings.at(CurrentGroup).fUseDynamicPassives)
        Passives->calculateDynamicPassives(iev, EventsDataHub->ReconstructionData.at(CurrentGroup).at(iev));
  }

  MW->Owindow->RefreshData();

  double range = ui->ledMapRange->text().toDouble();
  int bins = ui->sbMapBins->value();
  bool RecEnergy = ui->cbReconstructEnergy->isChecked();
  double Energy = ui->ledSuggestedEnergy->text().toDouble();
  bool WeightedChi2calculation = true;
  if (CurrentGroup < EventsDataHub->RecSettings.size())
      WeightedChi2calculation = EventsDataHub->RecSettings.at(CurrentGroup).fWeightedChi2calculation;

  double delta = range/bins;
  double x0, y0;
  switch (MapCenterOption)
    {
    case 0: //CoG
      x0 = EventsDataHub->ReconstructionData[CurrentGroup][iev]->xCoG - 0.5*delta*(bins-1);
      y0 = EventsDataHub->ReconstructionData[CurrentGroup][iev]->yCoG - 0.5*delta*(bins-1);
      break;
    case 1: //Scan
      x0 = EventsDataHub->Scan[iev]->Points[0].r[0] - 0.5*delta*(bins-1);
      y0 = EventsDataHub->Scan[iev]->Points[0].r[1] - 0.5*delta*(bins-1);
      break;
    case 2: //reconstructed
      x0 = EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[0] - 0.5*delta*(bins-1);
      y0 = EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[1] - 0.5*delta*(bins-1);
      break;
    default:
      qWarning()<<"Unknown map start option!";
      return;
    }

  auto hist2D = new TH2D("MapEvHist","",
                     bins, x0, x0+delta*bins,
                     bins, y0, y0+delta*bins);
  hist2D->GetXaxis()->SetTitle("X, mm");
  hist2D->GetYaxis()->SetTitle("Y, mm");

  MW->WindowNavigator->BusyOn();
  double Zmin=1e10;
  double Zmax=-1e10;
  for (int ix = 0; ix<bins; ix++)
    for (int iy = 0; iy<bins; iy++)
      {
        double x = x0 + delta*ix;
        double y = y0 + delta*iy;
        double val = 0; //chi2 or Ln(likelihood) in this point

        if (RecEnergy)
          {
            double sumSig = 0;
            double sumLRF = 0;
            for (int ipm=0; ipm<MW->PMs->count(); ipm++)
              if (Passives->isActive(ipm))
                {
                  sumSig += EventsDataHub->Events[iev][ipm];
                  sumLRF += Detector->LRFs->getLRF(ipm, x, y, z);
                }
            if (sumLRF>0) Energy = sumSig/sumLRF;
          }

        for (int ipm=0; ipm<MW->PMs->count(); ipm++)
          if (Passives->isActive(ipm))
            {
              //calculating LRF of this pm at this coordinates
              double LRFhere = Detector->LRFs->getLRF(ipm, x, y, z) * Energy;
              if (LRFhere == 0)
                {
                  //LRF is undefined here!
                  val = 0;
                  break;
                }

              //calculating the value of ci2 or LN(like) at this position
              if (ui->cobMapType->currentIndex() == 0)
                {
                  //chi2
                  double delta = (LRFhere - EventsDataHub->Events[iev][ipm]);

                  if (WeightedChi2calculation)
                    {
                      double sigma;
                      //sigma = LRFmodule->getSigma(ipm, X, Y) * sqrt(energy);
                      sigma = sqrt(LRFhere);

                      val += delta*delta/sigma/sigma;
                    }
                  else val += delta*delta;
                }
              else
                {
                  //ln max likelihood
                  val += EventsDataHub->Events[iev][ipm] * log(LRFhere) - LRFhere;
                }
            }

        if (val != 0)
          {
            hist2D->Fill(x, y, val);
            if (val>Zmax) Zmax = val;
            if (val<Zmin) Zmin = val;
          }
      }
  MW->WindowNavigator->BusyOff(false);

  hist2D->GetZaxis()->SetRangeUser(Zmin,Zmax);
  MW->GraphWindow->Draw(hist2D, "colz", false);

  //showing markers
  QVector<TLine*> Markers;

  double len = delta*0.06*bins; //marker length

  if (!EventsDataHub->isScanEmpty())
    {
      for (int ip=0; ip<EventsDataHub->Scan[iev]->Points.size(); ip++)
        {
          //triple whiter
          double x0 = EventsDataHub->Scan[iev]->Points[ip].r[0];
          double y0 = EventsDataHub->Scan[iev]->Points[ip].r[1];

          Markers << PrepareMarker(x0, y0, len, kBlue);
        }
    }

  if (EventsDataHub->ReconstructionData[CurrentGroup][iev]->ReconstructionOK)
    {
      for (int ip=0; ip<EventsDataHub->Scan[iev]->Points.size(); ip++)
        {
          //triple whiter
          double x0 = EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[ip].r[0];
          double y0 = EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[ip].r[1];

          Markers << PrepareMarker(x0, y0, len, kRed);
        }
    }

  for (int i=0; i<Markers.size(); i++)
      MW->GraphWindow->Draw(Markers[i], "same", false);

  MW->GraphWindow->UpdateRootCanvas();
  MW->GraphWindow->LastDistributionShown = "MapEvent";
  delete Passives;
}

QVector<TLine*> ReconstructionWindow::PrepareMarker(double x0, double y0, double len, Color_t color)
{
  QVector<TLine*> Markers;

  TLine* l1h = new TLine(x0-len,y0,x0+len,y0);
  l1h->SetLineWidth(3);
  l1h->SetLineColor(kWhite);
  Markers.append(l1h);
  TLine* l1v = new TLine(x0,y0-len,x0,y0+len);
  l1v->SetLineWidth(3);
  l1v->SetLineColor(kWhite);
  Markers.append(l1v);

  TLine* l2h = new TLine(x0-len,y0,x0+len,y0);
  l2h->SetLineWidth(2);
  l2h->SetLineColor(color);
  Markers.append(l2h);
  TLine* l2v = new TLine(x0,y0-len,x0,y0+len);
  l2v->SetLineWidth(1);
  l2v->SetLineColor(color);
  Markers.append(l2v);

  return Markers;
}

void ReconstructionWindow::on_cobMapZ_currentIndexChanged(int index)
{
    ui->ledMapFixedZ->setEnabled(!(bool)index);
}

void ReconstructionWindow::on_pbTogglePassives_clicked()
{
  if (dialog)
    {
      dialog->show();
      dialog->activateWindow();
      return;
    }

  dialog = new QDialog(this);
  dialog->setWindowTitle("Toggle active/passive PMs");
  dialog->resize(400,400);
  dialog->show();

  QPushButton *okButton = new QPushButton("Close");
  connect(okButton,SIGNAL(clicked()),dialog,SLOT(accept()));

  if (vo) delete vo;
  myQGraphicsView* gv = new myQGraphicsView(this);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(gv);
  mainLayout->addWidget(okButton);

  dialog->setLayout(mainLayout);

  vo = new Viewer2DarrayObject(gv, MW->PMs);
  gv->show();
  okButton->show();
  ReconstructionWindow::PaintPMs();

  vo->DrawAll();
  vo->ResetViewport();
  connect(vo, SIGNAL(PMselectionChanged(QVector<int>)), this, SLOT(onSelectionChange(QVector<int>)));

  dialog->exec();

  if (dialog) delete dialog;
  dialog = 0;
  if (vo) delete vo;
  vo = 0;
}

void ReconstructionWindow::onSelectionChange(QVector<int> selectionArray)
{
  //qDebug()<<"Selection change was triggered. PMs selected: "<<selectionArray;
//  qDebug()<<"If selected one PM, toggling PM passive";
  if (selectionArray.size() == 1)
    {
      int ipm = selectionArray.first();
      if (PMgroups->isStaticPassive(ipm))
           PMgroups->clearStaticPassive(ipm);
      else PMgroups->setStaticPassive(ipm);
    }
   onUpdatePassiveIndication();
   UpdateReconConfig(); //Config->JSON update
   ReconstructionWindow::PaintPMs();
   vo->DrawAll();
}

void ReconstructionWindow::PaintPMs()
{
  //MW->Reconstructor->clearAllDynamicPassives();
  for (int ipm=0; ipm<MW->PMs->count(); ipm++)
    {
      vo->SetText(ipm, QString::number(ipm));

      if (PMgroups->isPassive(ipm))
        {
          vo->SetBrushColor(ipm, Qt::black);
          vo->SetTextColor(ipm, Qt::white);
        }
      else
        {
          vo->SetBrushColor(ipm, Qt::white);
          vo->SetTextColor(ipm, Qt::black);
        }
    }
}

void ReconstructionWindow::UpdatePMGroupCOBs()
{
  UpdateOneGroupCOB(ui->cobAssignToPMgroup);
  UpdateOneGroupCOB(ui->cobCurrentGroup);
  bool fMultiple = (PMgroups->countPMgroups() > 1);
  ui->cobCurrentGroup->setEnabled(fMultiple);
  ui->cobCurrentGroup->setCurrentIndex(PMgroups->getCurrentGroup());
}

void ReconstructionWindow::UpdateOneGroupCOB(QComboBox* cob)
{
  QString old = cob->currentText();
  cob->clear();
  bool oldFound = false;

  for (int igroup=0; igroup<MW->Detector->PMgroups->countPMgroups(); igroup++)
    {
      QString str = MW->Detector->PMgroups->getGroupName(igroup);
      cob->addItem(str);

      if (str == old)
        {
          cob->setCurrentText(str);
          oldFound = true;
        }
    }

  if (!oldFound) cob->setCurrentIndex(0);
}

void ReconstructionWindow::on_cobShowWhat_currentIndexChanged(int /*index*/)
{
    bool show = false;
    if (ui->cobShowWhat->currentText() == "PM signal") show = true;

    ui->fShowXYPmnumber->setVisible(show);
}

void ReconstructionWindow::DoBlur(double sigma, int type)
{  
   QString str = QString::number(sigma);
   ui->cobBlurType->setCurrentIndex(type);
   if (type == 0) ui->ledBlurDelta->setText(str);
   else           ui->ledBlurSigma->setText(str);
   qApp->processEvents();
   ReconstructionWindow::on_pbBlurReconstructedXY_clicked();
}

void ReconstructionWindow::on_pbExportData_clicked()
{
    if (EventsDataHub->Events.isEmpty()) return;
    if (EventsDataHub->isReconstructionDataEmpty()) return;

    QString fileName = QFileDialog::getSaveFileName(this, "Export data", MW->GlobSet->LastOpenDir, "Data files (*.dat);;Text files (*.txt)");
    if (fileName.isEmpty()) return;
    MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();

    QFile outFile( fileName );
    outFile.open(QIODevice::WriteOnly);
    if(!outFile.isOpen())
      {
        message("Unable to open file " +fileName+ " for writing!", this);
        return;
      }
    QTextStream outStream(&outFile);

    for (int ievent=0; ievent < EventsDataHub->Events.size(); ievent++)
      if (EventsDataHub->ReconstructionData[0][ievent]->GoodEvent)
        {
          for (int ipm=0; ipm<MW->PMs->count(); ipm++)
            outStream << EventsDataHub->Events.at(ievent)[ipm] << " ";
          outStream << "\r\n";
        }
    outFile.close();
}

Double_t gauss(Double_t *x, Double_t *par)
{
   Double_t X = x[0];
   Double_t Y = x[1];
   Double_t x0 = par[0];
   Double_t y0 = par[1];
   Double_t w = par[2];
   Double_t Norm = par[3];

   Double_t r2 = (X-x0)*(X-x0) + (Y-y0)*(Y-y0);
   Double_t result = Norm * TMath::Exp(-r2/w);
   return result;
}

void ReconstructionWindow::on_pbCenterControlEvaluate_clicked()
{
  int ipm = ui->sbCenterControlPM->value();
  if (ipm < 0 || ipm > MW->PMs->count()-1)
    {
      message("Wrong PM number!", this);
      return;
    }

  int CurrentGroup = PMgroups->getCurrentGroup();
  if (!EventsDataHub->isReconstructionReady(CurrentGroup))
    {
      message("To use this feature, first perform \"Reconstruct all events\"", this);
      return;
    }

  ReconstructionWindow::GetCenterShift(ipm, true);
}

bool ReconstructionWindow::GetCenterShift(int ipm, bool fDraw)
{
  if (ipm < 0 || ipm > MW->PMs->count()-1) return false;
  int bins = ui->sbCenterControlBins->value();
  double radius = ui->ledCenterControlRadius->text().toDouble();
  double radius2 = radius*radius;
  int MethodOption = ui->cobCenterOption->currentIndex(); // 0- Gaussian, 1 - centroid

  double sumX = 0;
  double sumY = 0;
  double sumSig = 0;
  int iEvents = 0;
  int x0 = MW->PMs->X(ipm);
  int y0 = MW->PMs->Y(ipm);

  auto centHist = new TH2D("centrHist","", bins, -radius, radius, bins, -radius, radius);
  TH2D* centHistNorm = new TH2D("centrHistNorm","", bins, -radius, radius, bins, -radius, radius);

  for (int iev = 0; iev < EventsDataHub->ReconstructionData[0].size(); iev++)
   {
    if (EventsDataHub->ReconstructionData[0][iev]->Points.size() == 1)
      {
        if (! EventsDataHub->ReconstructionData[0][iev]->GoodEvent) continue;
        double x = EventsDataHub->ReconstructionData[0][iev]->Points[0].r[0] - x0;
        double y = EventsDataHub->ReconstructionData[0][iev]->Points[0].r[1] - y0;
        double r2 = x*x + y*y;
        if (r2 > radius2) continue;

        iEvents++;
        double sig = EventsDataHub->Events.at(iev).at(ipm);
        sumSig += sig;
        sumX += sig*x;
        sumY += sig*y;

        centHist->Fill(x, y, sig);
        centHistNorm->Fill(x, y);
      }
    }

  if (sumSig <= 0)
    {
      qDebug() << "ipm="<<ipm<< "Error: sum signal = " << sumSig;
      delete centHistNorm;
      return false;
    }

  sumX /= sumSig;
  sumY /= sumSig;

  *centHist = *centHist / *centHistNorm;

  //Gauss fit
  funcParams[0] = 0;
  funcParams[1] = 0;  
  TF2 *Gauss = new TF2("f2",gauss,-radius,radius,-radius,radius, 4);
  Gauss->SetParameters(funcParams);
  centHist->Fit("f2", "N");

  if (fDraw)
    {
      centHist->GetXaxis()->SetTitle("Delta from PM center X, mm");
      centHist->GetYaxis()->SetTitle("Delta from PM center Y, mm");

      MW->GraphWindow->Draw(centHist, "colz", false);
      MW->GraphWindow->Draw(Gauss, "same cont1");
    }

  if (MethodOption == 1)
    {
      //centroid
      funcParams[0] = sumX;
      funcParams[1] = sumY;
    }
  else
    {
      //gaussian
      funcParams[0] = Gauss->GetParameter(0);
      funcParams[1] = Gauss->GetParameter(1);
    }
  funcParams[2] = Gauss->GetParameter(2); //next will start from these data
  funcParams[3] = Gauss->GetParameter(3);

  qDebug() << "Events in radius, centroid x,y: "<< iEvents << sumX << sumY;
  qDebug() << "Gauss center x,y:" << Gauss->GetParameter(0) << Gauss->GetParameter(1);
  // x0 = par[0] y0 = par[1] w = par[2] Norm = par[3];

  delete centHistNorm;
  if (!fDraw)
  {
      delete centHist;
      delete Gauss;
  }
  return true;
}


void ReconstructionWindow::on_pbEvaluateAndShift_clicked()
{
   QMessageBox::StandardButton reply;
   reply = QMessageBox::question(this, "Confirmation needed", "Shift sensor position (no undone)?",
                                  QMessageBox::Yes|QMessageBox::Cancel);
   if (reply == QMessageBox::Cancel) return;

   int ipm = ui->sbCenterControlPM->value();
   ReconstructionWindow::DoCenterShift(ipm);
}

void ReconstructionWindow::DoCenterShift(int ipm)
{
   int CurrentGroup = PMgroups->getCurrentGroup();
   if (!EventsDataHub->isReconstructionReady(CurrentGroup))
     {
       message("To use this feature, first perform \"Reconstruct all events\"", this);
       return;
     }
   if (ipm < 0 || ipm > MW->PMs->count()-1)
     {
       message("Wrong PM number!", this);
       return;
     }

  ReconstructionWindow::GetCenterShift(ipm, false);
  double dx = funcParams[0]; //x shift from actual PM position
  double dy = funcParams[1];

  //shifting PM
  double r[3];
  r[0] = MW->PMs->X(ipm) + dx;
  r[1] = MW->PMs->Y(ipm) + dy;
  r[2] = MW->PMs->Z(ipm);
  MW->PMs->at(ipm).setCoords(r);
}

int ReconstructionWindow::getReconstructionAlgorithm()
{
  return ui->cobReconstructionAlgorithm->currentIndex();
}

void ReconstructionWindow::on_pbMakePictureVsXY_clicked()
{
    TObject* obj = MW->GraphWindow->GetMainPlottedObject();
    if (!obj || QString(obj->ClassName()) != "TH2D")
      {
        message("Histogram is not ready!", this);
        return;
      }

    TH2D* hist2D = static_cast<TH2D*>(obj);
    int binsX = hist2D->GetNbinsX();
    int binsY = hist2D->GetNbinsY();
    QImage image(binsX, binsY, QImage::Format_RGB16);
    QRgb value;

    double scale = ui->ledMakePictureScale->text().toDouble() / hist2D->GetMaximum();
    for (int iy=0; iy<binsY; iy++)   //0 - underflow, bins+1 overflow
      for (int ix=0; ix<binsX; ix++)
        {
          double level = hist2D->GetBinContent(ix+1, binsY-iy) * scale;
          int lev = level;
          if (lev > 255) lev = 255;
          value = qRgb(lev, lev, lev);
          image.setPixel(ix, iy, value);
        }

    image.save("showvsxy.png");
    QDesktopServices::openUrl(QUrl("file:showvsxy.png", QUrl::TolerantMode));
}

void ReconstructionWindow::on_pbShiftAll_clicked()
{
    QVector<double> dx, dy;

    //calculating all shifts
    for (int ipm=0; ipm<MW->PMs->count(); ipm++)
    {
        bool OK = ReconstructionWindow::GetCenterShift(ipm, false);
        if (OK)
        {
            dx.append(funcParams[0]); //x shift from actual PM position
            dy.append(funcParams[1]);
        }
        else
        {
            dx.append(0);
            dy.append(0);
        }
        qDebug() << "pm:"<<ipm<<dx.last()<<dy.last();
    }

    //shifting PM
    double r[3];
    for (int ipm=0; ipm<MW->PMs->count(); ipm++)
    {
        r[0] = MW->PMs->X(ipm) + dx[ipm];
        r[1] = MW->PMs->Y(ipm) + dy[ipm];
        r[2] = MW->PMs->Z(ipm);
        MW->PMs->at(ipm).setCoords(r);
    }
}

void ReconstructionWindow::on_pbShiftAllWithDis_clicked()
{
    QVector<double> dx, dy;

    //calculating all shifts
    for (int ipm=0; ipm<MW->PMs->count(); ipm++)
    {
        PMgroups->setTMPpassive(ipm);
        ReconstructionWindow::on_pbReconstructAll_clicked();
        PMgroups->clearTMPpassive(ipm);

        bool OK = ReconstructionWindow::GetCenterShift(ipm, false);
        if (OK)
        {
            dx.append(funcParams[0]); //x shift from actual PM position
            dy.append(funcParams[1]);
        }
        else
        {
            dx.append(0);
            dy.append(0);
        }
        qDebug() << "pm:"<<ipm<<dx.last()<<dy.last();
    }

    //shifting PM
    double r[3];
    for (int ipm=0; ipm<MW->PMs->count(); ipm++)
    {
        r[0] = MW->PMs->X(ipm) + dx[ipm];
        r[1] = MW->PMs->Y(ipm) + dy[ipm];
        r[2] = MW->PMs->Z(ipm);
        MW->PMs->at(ipm).setCoords(r);
    }
}

void ReconstructionWindow::on_pbEvaluateGrid_clicked()
{
   int ipm = ui->sbCenterControlPM->value();

   double dx, dy;
   ReconstructionWindow::EvaluateShiftBrute(ipm, &dx, &dy);
}

void ReconstructionWindow::EvaluateShiftBrute(int ipm, double* dx, double* dy)
{
  int steps = ui->sbCenterStepsLR->value();
  double step = ui->ledCenterStep->text().toDouble();

  double r0[3], r[3];
  r0[0] = MW->PMs->X(ipm);
  r0[1] = MW->PMs->Y(ipm);
  r0[2] = r[2] = MW->PMs->Z(ipm);
  QVector< QVector <double> > chi2;
  chi2.resize(steps*2+1);

  TH2D* h = new TH2D("centerTest","",steps*2+1,r0[0] - step*(steps+0.5),r0[0] + step*(steps+0.5),
                                     steps*2+1,r0[1] - step*(steps+0.5),r0[1] + step*(steps+0.5));
  double bestChi2=1e10;
  int best_ix=0, best_iy=0;

  for (int iy=-steps; iy<steps+1; iy++)
    for (int ix=-steps; ix<steps+1; ix++)
      {
        //shifting PM
        r[0] = r0[0] + ix*step;
        r[1] = r0[1] + iy*step;
        MW->PMs->at(ipm).setCoords(r);
        qDebug() << "New coords:"<<MW->PMs->X(ipm)<<MW->PMs->Y(ipm);

        ReconstructionWindow::on_pbReconstructAll_clicked();
        qDebug() << "----> chi2:"<<lastChi2;
        chi2[iy+steps].append(lastChi2);
        h->Fill(r[0],r[1],lastChi2);

        if (lastChi2 < bestChi2)
        {
            bestChi2 = lastChi2;
            best_ix = ix;
            best_iy = iy;
        }
      }

  MW->PMs->at(ipm).setCoords(r0); //returning back to original coordinates
  qDebug() << "---->Best Chi2 found at indexes:" << best_ix<<best_iy << "and chi2="<<bestChi2;
  *dx = best_ix*step;
  *dy = best_iy*step;

  MW->GraphWindow->Draw(h,"surf");
  qApp->processEvents();
}

double ReconstructionWindow::getSuggestedZ() const
{
  return ui->ledSuggestedZ->text().toDouble();
}

double ReconstructionWindow::isReconstructEnergy() const
{
  return ui->cbReconstructEnergy->isChecked();
}

void ReconstructionWindow::on_pbEvAllBruteThenShift_clicked()
{
    QVector <double> dX, dY;

    for (int ipm=0; ipm<MW->PMs->count(); ipm++)
      {
        qDebug() << "-----------------------------\nPM#:" << ipm;
        double dx, dy;
        ReconstructionWindow::EvaluateShiftBrute(ipm, &dx, &dy);
        qDebug() << "shifts:"<<dX<<dY;
        dX.append(dx);
        dY.append(dy);
      }

    qDebug() << "Shifting all PMs";
    for (int ipm=0; ipm<MW->PMs->count(); ipm++)
      {
        double r[3];
        r[0] = MW->PMs->X(ipm) + dX[ipm];
        r[1] = MW->PMs->Y(ipm) + dY[ipm];
        r[2] = MW->PMs->Z(ipm);

        MW->PMs->at(ipm).setCoords(r);
      }
}

void ReconstructionWindow::on_pbSensorGainsToPMgains_clicked()
{
   message("Not supported anymore", this);
   return;
   /*
   if (!MW->SensLRF->isAllLRFsDefined())
     {
       message("LRFs are not defined!", this);
       return;
     }

   for (int ipm=0; ipm<MW->PMs->count(); ipm++)
     {
       double gain = MW->SensLRF->getSensor(ipm)->GetGain();
       MW->PMs->at(ipm).relGain = gain;
     }
   ReconstructionWindow::on_pbShowAllGainsForGroup_clicked();
   UpdateReconConfig(); //Config->JSON update
   */
}

void ReconstructionWindow::RefreshOnTimer(int Progress, double usPerEv)
{  
  SetProgress(Progress);
  if (usPerEv == 0) ui->leoMsPerEv->setText("n.a.");
  else ui->leoMsPerEv->setText(QString::number(usPerEv, 'g', 4));

  qApp->processEvents();
  if (ui->pbStopReconstruction->isChecked())
    {
      emit StopRequested();
      return;
    }
}

void ReconstructionWindow::updateReconSettings()
{
  QJsonObject RecJson;

  //general
  QJsonObject gjson;
    gjson["ReconstructEnergy"] = ui->cbReconstructEnergy->isChecked();
    gjson["InitialEnergy"] = ui->ledSuggestedEnergy->text().toDouble();
    gjson["ReconstructZ"] = ui->cb3Dreconstruction->isChecked();
    gjson["Zstrategy"] = ui->cobZ->currentIndex();
    gjson["InitialZ"] = ui->ledSuggestedZ->text().toDouble();
    gjson["IncludePassives"] = ui->cbIncludePassives->isChecked();
    gjson["WeightedChi2"] = ui->cbWeightedChi2->isChecked();
    gjson["LimitSearchIfTrueIsSet"] = ui->cbLimitSearchToVicinity->isChecked();
    gjson["RangeForLimitSearchIfTrueSet"] = ui->ledLimitSearchRange->text().toDouble();
    gjson["LimitSearchGauss"] = ui->cbGaussWeightInMinimization->isChecked();
  RecJson["General"] = gjson;

  //Algotithm
  QJsonObject ajson;
  int iAlg = ui->cobReconstructionAlgorithm->currentIndex(); //0-CoG, 1-CG cpu, 2-Root, 3-SNN, 4-CUDA
  ajson["Algorithm"] = iAlg;
    //cog
  QJsonObject cogjson;
      cogjson["ForceFixedZ"] = ui->cbForceCoGgiveZof->isChecked();
      cogjson["DoStretch"] = ui->cbCoGapplyStretch->isChecked();
      cogjson["StretchX"] = ui->ledCoGstretchX->text().toDouble();
      cogjson["StretchY"] = ui->ledCoGstretchY->text().toDouble();
      cogjson["StretchZ"] = ui->ledCoGstretchZ->text().toDouble();
      cogjson["IgnoreBySignal"] = ui->cbCogIgnoreLow->isChecked(); //independent from DynamicPassive - can use different settings
      cogjson["IgnoreThresholdLow"] = ui->ledCoGignoreLevelLow->text().toDouble();
      cogjson["IgnoreThresholdHigh"] = ui->ledCoGignoreLevelHigh->text().toDouble();
      cogjson["IgnoreFar"] = ui->cbCoGIgnoreFar->isChecked();
      cogjson["IgnoreDistance"] = ui->ledCoGignoreThreshold->text().toDouble();
  ajson["CoGoptions"] = cogjson;
      //algorithm specific
  // ContrGrids
  QJsonObject gcpuJson;
        gcpuJson["StartOption"] = ui->cobCGstartOption->currentIndex();
        gcpuJson["OptimizeWhat"] = ui->cobCGoptimizeWhat->currentIndex();
        gcpuJson["NodesXY"] = ui->sbCGnodes->value();
        gcpuJson["Iterations"] = ui->sbCGiter->value();
        gcpuJson["InitialStep"] = ui->ledCGstartStep->text().toDouble();
        gcpuJson["Reduction"] = ui->ledCGreduction->text().toDouble();       
  ajson["CPUgridsOptions"] = gcpuJson;

  // Root minimizer
  QJsonObject rootJson;
        rootJson["StartOption"] = ui->cobLSstartingXY->currentIndex();
        rootJson["Minuit2Option"] = ui->cobMinuit2Option->currentIndex();
        rootJson["LSorLikelihood"] = ui->cobLSminimizeWhat->currentIndex();
        rootJson["StartStepX"] = ui->ledInitialStepX->text().toDouble();
        rootJson["StartStepY"] = ui->ledInitialStepY->text().toDouble();
        rootJson["StartStepZ"] = ui->ledInitialStepZ->text().toDouble();
        rootJson["StartStepEnergy"] = ui->ledInitialStepEnergy->text().toDouble();
        rootJson["MaxCalls"] = ui->sbLSmaxCalls->value();
        rootJson["LSsuppressConsole"] = ui->cbLSsuppressConsole->isChecked();
  ajson["RootMinimizerOptions"] = rootJson;

  // ANN
#ifdef ANTS_FANN
        QJsonObject fannJson;
        MW->NNwindow->saveJSON(fannJson);
        ajson["FANNsettings"] = fannJson;
#endif

  // CUDA
  QJsonObject cudaJson;
      cudaJson["ThreadBlockXY"] = ui->sbCUDAthreadBlockSize->value();
      cudaJson["StartStep"] = ui->ledCUDAstartStep->text().toFloat();
      cudaJson["Iterations"] = ui->sbCUDAiterations->value();
      cudaJson["ScaleReduction"] = ui->ledCUDAscaleReductionFactor->text().toFloat();
      cudaJson["StartOption"] = ui->cobCUDAoffsetOption->currentIndex();
      cudaJson["StartX"] = ui->ledCUDAxoffset->text().toFloat();
      cudaJson["StartY"] = ui->ledCUDAyoffset->text().toFloat();
      cudaJson["OptimizeMLChi2"] = ui->cobCUDAminimizeWhat->currentIndex();
      //cudaJson["Passives"] = ui->cbCUDApassives->isChecked();
      //cudaJson["PassiveOption"] = ui->cobCUDApassiveOption->currentIndex();
      //cudaJson["Threshold"] = ui->ledCUDAthreshold->text().toFloat();
      //cudaJson["MaxDistance"] = ui->ledCUDAmaxDistance->text().toFloat();
      cudaJson["StarterZ"] = ui->ledSuggestedZ->text().toFloat();
  ajson["CGonCUDAsettings"] = cudaJson;

  // kNN
#ifdef ANTS_FLANN
        QJsonObject kNNrecJson;
        kNNrecJson["numNeighbours"] = ui->sbKNNnumNeighbours->value();
        kNNrecJson["numTrees"] = ui->sbKNNnumTrees->value();
        kNNrecJson["calibScheme"] = ui->cobKNNcalibration->currentIndex(); //only gui
        kNNrecJson["useNeighbours"] = ui->sbKNNuseNeighboursInRec->value();
        kNNrecJson["weightMode"] = ui->cobKNNweighting->currentIndex();
        ajson["kNNrecSet"] = kNNrecJson;
#endif

  RecJson["AlgorithmOptions"] = ajson;

    //Dynamic passives - applicable for CGonCPU, Root and CUDA
  QJsonObject dynPassJson;
    dynPassJson["IgnoreBySignal"] = ui->cbDynamicPassiveBySignal->isChecked();
    dynPassJson["SignalLimitLow"] = ui->ledDynamicPassiveThresholdLow->text().toDouble();
    dynPassJson["SignalLimitHigh"] = ui->ledDynamicPassiveThresholdHigh->text().toDouble();
    dynPassJson["IgnoreByDistance"] = ui->cbDynamicPassiveByDistance->isChecked();
    dynPassJson["DistanceLimit"] = ui->ledPassiveRange->text().toDouble();
  RecJson["DynamicPassives"] = dynPassJson;

    //Multiples
  QJsonObject multJson;
  multJson["Option"] = ui->cobMultipleOption->currentIndex();
  RecJson["MultipleEvents"] = multJson;  

    //Limit nodes
  if (ui->cbInRecActivate->isChecked())
    {
      QJsonObject lnJson;
      lnJson["Active"] = true;
      lnJson["Shape"] = ui->cobInRecShape->currentIndex();
      lnJson["Size1"] = ui->ledInRecSize1->text().toDouble();
      lnJson["Size2"] = ui->ledInRecSize2->text().toDouble();
      RecJson["LimitNodes"] = lnJson;
    }

  //updating configuration JSON
  MW->Detector->Config->UpdateReconstructionSettings(RecJson, PMgroups->getCurrentGroup());
}

bool ReconstructionWindow::readReconSettingsFromJson(QJsonObject &jsonMaster)
{
  if (!jsonMaster.contains("ReconstructionOptions"))
    {
      message("--- Json does not contain reconstruction options!");
      return false;
    }

  //old system (no sensor groups) - ReconstructionOptions is QJsonObject
  //new system - it is QJsonArray of QJsonObjects, select the one corresponding to the CurrentGroup of PMgroups
  QJsonObject RecJson;
  if (jsonMaster["ReconstructionOptions"].isArray())
  {
      //new
      QJsonArray ar = jsonMaster["ReconstructionOptions"].toArray();
      if (ar.size() != MW->Detector->PMgroups->countPMgroups())
      {
          message("Size of json array with reconstr settings != number of defined groups!", this);
          return false;
      }

      int current = PMgroups->getCurrentGroup();
      if (current<0 || current > ar.size()-1)
      {
          message("!!! Json does not contain filter settings for current sensor group", this);
          return false;
      }

      RecJson = ar[current].toObject();
  }
  else
  {
      //old
      qWarning() << "--- Old system of Sensor Groups detected - dublicating recon settings if necessary";
      RecJson = jsonMaster["ReconstructionOptions"].toObject();
  }

  //general
  QJsonObject gjson = RecJson["General"].toObject();
  JsonToCheckbox(gjson, "ReconstructEnergy", ui->cbReconstructEnergy);
  JsonToLineEdit(gjson, "InitialEnergy", ui->ledSuggestedEnergy);
  JsonToCheckbox(gjson, "ReconstructZ", ui->cb3Dreconstruction);
  JsonToLineEdit(gjson, "InitialZ", ui->ledSuggestedZ);
  ui->cobZ->setCurrentIndex(0); //compatibility
  JsonToComboBox(gjson, "Zstrategy", ui->cobZ);
  JsonToCheckbox(gjson, "IncludePassives", ui->cbIncludePassives);
  JsonToCheckbox(gjson, "WeightedChi2", ui->cbWeightedChi2);  
  ui->cbLimitSearchToVicinity->setChecked(false); //compatibility
  JsonToCheckbox(gjson, "LimitSearchIfTrueIsSet", ui->cbLimitSearchToVicinity);
  JsonToLineEdit(gjson, "RangeForLimitSearchIfTrueSet", ui->ledLimitSearchRange);  
  JsonToCheckbox(gjson, "LimitSearchGauss", ui->cbGaussWeightInMinimization);

  //Dynamic passives - before algorithms for compatibility: CUDA settings can overrite them if old file is loaded
  if (RecJson.contains("DynamicPassives"))
    {
      QJsonObject dynPassJson = RecJson["DynamicPassives"].toObject();
      JsonToCheckbox(dynPassJson, "IgnoreBySignal", ui->cbDynamicPassiveBySignal);
      JsonToLineEdit(dynPassJson, "SignalLimitLow", ui->ledDynamicPassiveThresholdLow);
      JsonToLineEdit(dynPassJson, "SignalLimitHigh", ui->ledDynamicPassiveThresholdHigh);
      JsonToCheckbox(dynPassJson, "IgnoreByDistance", ui->cbDynamicPassiveByDistance);
      JsonToLineEdit(dynPassJson, "DistanceLimit", ui->ledPassiveRange);
    }
  else
    {
      //compatibility
      ui->cbDynamicPassiveBySignal->setChecked(false);
      ui->ledDynamicPassiveThresholdLow->setText("5");
      ui->ledDynamicPassiveThresholdHigh->setText("1e10");
      ui->cbDynamicPassiveByDistance->setChecked(false);
      ui->ledPassiveRange->setText("100");
      //before data were group-related, not anymore
      //Extracting old data assuming it is group 0
//      QJsonObject pmJson = jsonMaster["PMrelatedSettings"].toObject();
//      if (pmJson.contains("Groups"))
//        {
//          QJsonObject grJson = pmJson["Groups"].toObject();

//          if (grJson.contains("DynamicPassiveThreshold")) //compatibility
//            {
//              QJsonArray DynamicPassiveThresholds = grJson["DynamicPassiveThreshold"].toArray();
//              if (DynamicPassiveThresholds.size()>0)
//                {
//                  double thr = DynamicPassiveThresholds[0].toDouble();
//                  ui->ledDynamicPassiveThresholdLow->setText(QString::number(thr));
//                }
//            }
//          if (grJson.contains("DynamicPassiveRanges"))
//            {
//              QJsonArray DynamicPassiveRanges = grJson["DynamicPassiveRanges"].toArray();
//              if (DynamicPassiveRanges.size()>0)
//                {
//                  double range = DynamicPassiveRanges[0].toDouble();
//                  ui->ledPassiveRange->setText(QString::number(range));
//                }
//            }
//        }
    }

  //Algotithm
  QJsonObject ajson = RecJson["AlgorithmOptions"].toObject();
  JsonToComboBox(ajson, "Algorithm", ui->cobReconstructionAlgorithm);//0-CoG, 1-CG cpu, 2-Root, 3-ANN, 4-CUDA, 5-kNN
    //cog
  QJsonObject cogjson = ajson["CoGoptions"].toObject();
  JsonToCheckbox(cogjson, "ForceFixedZ", ui->cbForceCoGgiveZof);
  JsonToCheckbox(cogjson, "DoStretch", ui->cbCoGapplyStretch);
  JsonToLineEdit(cogjson, "StretchX", ui->ledCoGstretchX);
  JsonToLineEdit(cogjson, "StretchY", ui->ledCoGstretchY);
  JsonToLineEdit(cogjson, "StretchZ", ui->ledCoGstretchZ);
  ui->cbCogIgnoreLow->setChecked(false);
  JsonToCheckbox(cogjson, "IgnoreLow", ui->cbCogIgnoreLow); //compatibility
  JsonToCheckbox(cogjson, "IgnoreBySignal", ui->cbCogIgnoreLow);
  JsonToLineEdit(cogjson, "IgnoreThreshold", ui->ledCoGignoreLevelLow); //compatibility
  JsonToLineEdit(cogjson, "IgnoreThresholdLow", ui->ledCoGignoreLevelLow);
  ui->ledCoGignoreLevelHigh->setText("1e10"); //compatibility
  JsonToLineEdit(cogjson, "IgnoreThresholdHigh", ui->ledCoGignoreLevelHigh);
  ui->cbCoGIgnoreFar->setChecked(false); //compatibility
  JsonToCheckbox(cogjson, "IgnoreFar", ui->cbCoGIgnoreFar);
  JsonToLineEdit(cogjson, "IgnoreDistance", ui->ledCoGignoreThreshold);
    //MG
  QJsonObject gcpuJson = ajson["CPUgridsOptions"].toObject();
  JsonToComboBox(gcpuJson, "StartOption", ui->cobCGstartOption);
  JsonToComboBox(gcpuJson, "OptimizeWhat", ui->cobCGoptimizeWhat);
  JsonToSpinBox(gcpuJson, "NodesXY", ui->sbCGnodes);
  JsonToSpinBox(gcpuJson, "Iterations", ui->sbCGiter);
  JsonToLineEdit(gcpuJson, "InitialStep", ui->ledCGstartStep);
  JsonToLineEdit(gcpuJson, "Reduction", ui->ledCGreduction);
  //JsonToCheckbox(gcpuJson, "DynamicPassives", ui->cbDynamicPassives);
  //JsonToComboBox(gcpuJson, "PassiveType", ui->cobDynamicPassiveType);
  //compatibility
  if (gcpuJson.contains("DynamicPassives"))
    {
      int SignalDist = 0;
      parseJson(gcpuJson, "PassiveType", SignalDist);
      if (SignalDist==0) ui->cbDynamicPassiveBySignal->setChecked(true);
      else ui->cbDynamicPassiveByDistance->setChecked(true);
    }
  //JsonToCheckbox(gcpuJson, "DynamicPassivesDistance", ui->cbDynamicPassiveByDistance);
  //JsonToCheckbox(gcpuJson, "DynamicPassivesSignal", ui->cbDynamicPassiveBySignal);
    //Minuit
  QJsonObject rootJson = ajson["RootMinimizerOptions"].toObject();
  JsonToComboBox(rootJson, "StartOption", ui->cobLSstartingXY);
  JsonToComboBox(rootJson, "Minuit2Option", ui->cobMinuit2Option);
  JsonToComboBox(rootJson, "LSorLikelihood", ui->cobLSminimizeWhat);
  JsonToLineEdit(rootJson, "StartStepX", ui->ledInitialStepX);
  JsonToLineEdit(rootJson, "StartStepY", ui->ledInitialStepY);
  JsonToLineEdit(rootJson, "StartStepZ", ui->ledInitialStepZ);
  JsonToLineEdit(rootJson, "StartStepEnergy", ui->ledInitialStepEnergy);
  JsonToSpinBox(rootJson, "MaxCalls", ui->sbLSmaxCalls);
  JsonToCheckbox(rootJson, "LSsuppressConsole", ui->cbLSsuppressConsole);
  //JsonToCheckbox(rootJson, "DynamicPassives", ui->cbDynamicPassives);
  //JsonToComboBox(rootJson, "PassiveType", ui->cobDynamicPassiveType);
  //compatibility
  if (rootJson.contains("DynamicPassives"))
    {
      int SignalDist = 0;
      parseJson(rootJson, "PassiveType", SignalDist);
      if (SignalDist==0) ui->cbDynamicPassiveBySignal->setChecked(true);
      else ui->cbDynamicPassiveByDistance->setChecked(true);
    }
  //JsonToCheckbox(rootJson, "DynamicPassivesDistance", ui->cbDynamicPassiveByDistance);
  //JsonToCheckbox(rootJson, "DynamicPassivesSignal", ui->cbDynamicPassiveBySignal);
    //ANN
#ifdef ANTS_FANN
  QJsonObject fannJson = ajson["FANNsettings"].toObject();
  MW->NNwindow->saveJSON(fannJson);
#endif
    // CUDA
  QJsonObject cudaJson = ajson["CGonCUDAsettings"].toObject();
  JsonToSpinBox(cudaJson, "ThreadBlockXY", ui->sbCUDAthreadBlockSize);
  JsonToSpinBox(cudaJson, "Iterations", ui->sbCUDAiterations);
  JsonToLineEdit(cudaJson, "StartStep", ui->ledCUDAstartStep);
  JsonToLineEdit(cudaJson, "ScaleReduction", ui->ledCUDAscaleReductionFactor);
  JsonToLineEdit(cudaJson, "StartX", ui->ledCUDAxoffset);
  JsonToLineEdit(cudaJson, "StartY", ui->ledCUDAyoffset);
  JsonToComboBox(cudaJson, "StartOption", ui->cobCUDAoffsetOption);
  JsonToComboBox(cudaJson, "OptimizeMLChi2", ui->cobCUDAminimizeWhat);
  //compatibility
  if (cudaJson.contains("Threshold"))
    {
      if (ui->cobReconstructionAlgorithm->currentIndex()==4)
        {
           //old system and this is the selected algorithm, so dynamic passives configuration has to be extracted
           ui->cbDynamicPassiveBySignal->setChecked(false);
           ui->cbDynamicPassiveByDistance->setChecked(false);
           bool fPassiveOn;
           parseJson(cudaJson, "Passives", fPassiveOn);
           if (fPassiveOn)
             {
               int SignalDistance=0;
               parseJson(cudaJson, "PassiveOption", SignalDistance);
               if (SignalDistance==0) ui->cbDynamicPassiveBySignal->setChecked(true);
               else ui->cbDynamicPassiveByDistance->setChecked(true);
               JsonToLineEdit(cudaJson, "Threshold", ui->ledDynamicPassiveThresholdLow);
               ui->ledDynamicPassiveThresholdHigh->setText("1e10");
               JsonToLineEdit(cudaJson, "MaxDistance", ui->ledPassiveRange);
             }
        }
    }

  //kNNrec
#ifdef ANTS_FLANN
  QJsonObject kNNrecJson = ajson["kNNrecSet"].toObject();
  JsonToSpinBox(kNNrecJson, "numNeighbours", ui->sbKNNnumNeighbours);
  JsonToSpinBox(kNNrecJson, "useNeighbours", ui->sbKNNuseNeighboursInRec);
  JsonToSpinBox(kNNrecJson, "numTrees", ui->sbKNNnumTrees);
  JsonToComboBox(kNNrecJson, "weightMode", ui->cobKNNweighting);
  JsonToComboBox(kNNrecJson, "calibScheme", ui->cobKNNcalibration); //only gui
#endif
    //Multiples
  QJsonObject multJson = RecJson["MultipleEvents"].toObject();
  JsonToComboBox(multJson, "Option", ui->cobMultipleOption);
    //Limit nodes
  ui->cbInRecActivate->setChecked(false); //compatibility
  QJsonObject lnJson = RecJson["LimitNodes"].toObject();
  if (!lnJson.isEmpty())
  {
    JsonToCheckbox(lnJson, "Active", ui->cbInRecActivate);
    JsonToComboBox(lnJson, "Shape", ui->cobInRecShape);
    JsonToLineEdit(lnJson, "Size1", ui->ledInRecSize1);
    JsonToLineEdit(lnJson, "Size2", ui->ledInRecSize2);
  }

  return true;
}

void ReconstructionWindow::updateFilterSettings()
{
  QJsonObject FiltJson;

  //event number
  QJsonObject njson;
      njson["Active"] = ui->cbFilterEventNumber->isChecked();
      njson["Min"] = ui->leiFromNE->text().toDouble();
      njson["Max"] = ui->leiToNE->text().toDouble();     
  FiltJson["EventNumber"] = njson;

  FiltJson["MultipleScanEvents"] = ui->cbFilterMultipleScanEvents->isChecked();

  //sum signal
  QJsonObject ssjson;
      ssjson["Active"] = ui->cbFilterSumSignal->isChecked();
      ssjson["UseGains"] = ui->cbGainsConsideredInFiltering->isChecked();
      ssjson["UsePassives"] = ui->cbPassivePMsTakenAccount->isChecked();
      ssjson["Min"] = ui->ledFilterSumMin->text().toDouble();
      ssjson["Max"] = ui->ledFilterSumMax->text().toDouble();
  FiltJson["SumSignal"] = ssjson;

  //indi signal
  QJsonObject sjson; //cut offs are individual and are transferred through PMs json
      sjson["Active"] = ui->cbFilterIndividualSignals->isChecked();
      sjson["UsePassives"] = ui->cbPassivePMsTakenAccount->isChecked();
  FiltJson["IndividualPMSignal"] = sjson;

  //reconstructed energy
  QJsonObject rejson;
      rejson["Active"] = ui->cbActivateEnergyFilter->isChecked();
      rejson["Min"] = ui->ledEnergyMin->text().toDouble();
      rejson["Max"] = ui->ledEnergyMax->text().toDouble();
  FiltJson["ReconstructedEnergy"] = rejson;

  //loaded energy
  QJsonObject lejson;
      lejson["Active"] = ui->cbActivateLoadedEnergyFilter->isChecked();
      lejson["Min"] = ui->ledLoadEnergyMin->text().toDouble();
      lejson["Max"] = ui->ledLoadEnergyMax->text().toDouble();
  FiltJson["LoadedEnergy"] = lejson;

  //chi2 
  QJsonObject cjson;
      cjson["Active"] = ui->cbActivateChi2Filter->isChecked();
      cjson["Min"] = ui->ledChi2Min->text().toDouble();
      cjson["Max"] = ui->ledChi2Max->text().toDouble();
  FiltJson["Chi2"] = cjson;

  //spatial - limit to a given object
  QJsonObject spfPjson;
      spfPjson["Active"] = ui->cbSpF_LimitToObject->isChecked();
      spfPjson["RecOrScan"] = ui->cobSpFRecScan->currentIndex();
      spfPjson["LimitObject"] = ui->leSpF_LimitToObject->text();
  FiltJson["SpatialToObject"] = spfPjson;

  //spatial - custom
  QJsonObject spfCjson;
      spfCjson["Active"] = ui->cbSpFcustom->isChecked();
      spfCjson["RecOrScan"] = ui->cobSpFRecScan->currentIndex();
      spfCjson["Shape"] = ui->cobSpFXY->currentIndex();
      spfCjson["SizeX"] = ui->ledSpFsizeX->text().toDouble();
      spfCjson["SizeY"] = ui->ledSpFsizeY->text().toDouble();
      spfCjson["Diameter"] = ui->ledSpFdiameter->text().toDouble();
      spfCjson["Side"] = ui->ledSpFside->text().toDouble();
      spfCjson["Angle"] = ui->ledSpFangle->text().toDouble();
      spfCjson["X0"] = ui->ledSpF_X0->text().toDouble();
      spfCjson["Y0"] = ui->ledSpF_Y0->text().toDouble();
      QJsonArray polyarr;
      if (ui->cobSpFXY->currentIndex() == 3)
      for (int i=0; i<polygon.size(); i++)
        {
          QJsonArray el;
          el.append( polygon[i].x() );
          el.append( polygon[i].y() );
          polyarr.append(el);
        }
      spfCjson["Polygon"] = polyarr;
      spfCjson["CutOutsideInside"] = ui->cobSpF_cutOutsideInside->currentIndex();
      spfCjson["AllZ"] = ui->cbSpFallZ->isChecked();
      spfCjson["Zfrom"] = ui->ledSpFfromZ->text().toDouble();
      spfCjson["Zto"] = ui->ledSpFtoZ->text().toDouble();
  FiltJson["SpatialCustom"] = spfCjson;

  //correlation
  QJsonObject corjson;
      corjson["Active"] = ui->cbActivateCorrelationFilters->isChecked();
      QJsonArray jsonArray;
      for (int ifil=0; ifil<CorrelationFilters.size(); ifil++)
        {
          QJsonObject jsonCorrFilt;
          CorrelationFilters[ifil]->saveJSON(jsonCorrFilt);
          jsonArray.append(jsonCorrFilt);
        }      
      corjson["Set"] = jsonArray;
  FiltJson["Correlation"] = corjson;

  //kNN
  QJsonObject knnjson;
      knnjson["Active"] = ui->cbActivateNNfilter->isChecked();
      knnjson["Min"] = ui->ledNNmin->text().toDouble();
      knnjson["Max"] = ui->ledNNmax->text().toDouble();
      knnjson["AverageOver"] = ui->sbNNaverageOver->value();
  FiltJson["kNN"] = knnjson;

  //updating Config JSON
  MW->Detector->Config->UpdateFilterSettings(FiltJson, PMgroups->getCurrentGroup());
}

bool ReconstructionWindow::readFilterSettingsFromJson(QJsonObject &jsonMaster)
{
  if (!jsonMaster.contains("FilterOptions"))
    {
      message("--- Json does not contain filter settings");
      return false;
    }

  bool fIgnore = TMPignore;
  TMPignore = true; //--//
  bool fOK = true;

  //old system (no sensor groups) - FilterOptions is QJsonObject
  //new system - it is QJsonArray of QJsonObjects, select the one corresponding to the CurrentGroup of PMgroups
  QJsonObject FiltJson;
  if (jsonMaster["FilterOptions"].isArray())
  {
      //new
      QJsonArray ar = jsonMaster["FilterOptions"].toArray();
      if (ar.size() != MW->Detector->PMgroups->countPMgroups())
      {
          message("Size of json array with filter settings != number of defined groups!", this);
          return false;
      }

      int current = PMgroups->getCurrentGroup();
      if (current<0 || current > ar.size()-1)
      {
          message("!!! Json does not contain filter settings for current sensor group", this);
          return false;
      }

      FiltJson = ar[current].toObject();
  }
  else
  {
      //old
      qWarning() << "--- Old system of Sensor Groups detected - dublicating filter settings if necessary";
      FiltJson = jsonMaster["FilterOptions"].toObject();
  }

  //comaptibility -> OLD save system: a particular filter record missing = filter inactive

  bool flag = FiltJson.contains("EventNumber"); //event number
  ui->cbFilterEventNumber->setChecked(flag);
  if (flag)
    {
      QJsonObject njson = FiltJson["EventNumber"].toObject();
      JsonToCheckbox(njson, "Active", ui->cbFilterEventNumber);
      JsonToLineEdit(njson, "Min", ui->leiFromNE);
      JsonToLineEdit(njson, "Max", ui->leiToNE);
    }  

  ui->cbFilterMultipleScanEvents->setChecked(false);
  JsonToCheckbox(FiltJson, "MultipleScanEvents", ui->cbFilterMultipleScanEvents);

  flag = FiltJson.contains("SumSignal"); //sum signal
  ui->cbFilterSumSignal->setChecked(flag);
  if (flag)
    {
      QJsonObject ssjson = FiltJson["SumSignal"].toObject();
      JsonToCheckbox(ssjson, "Active", ui->cbFilterSumSignal);
      JsonToCheckbox(ssjson, "UseGains", ui->cbGainsConsideredInFiltering);
      JsonToCheckbox(ssjson, "UsePassives", ui->cbPassivePMsTakenAccount);
      JsonToLineEdit(ssjson, "Min", ui->ledFilterSumMin);
      JsonToLineEdit(ssjson, "Max", ui->ledFilterSumMax);
    }

  flag = FiltJson.contains("IndividualPMSignal"); //indi signal
  ui->cbFilterIndividualSignals->setChecked(flag);
  if (flag)
    {      
      QJsonObject sjson = FiltJson["IndividualPMSignal"].toObject();
      JsonToCheckbox(sjson, "Active", ui->cbFilterIndividualSignals);
      JsonToCheckbox(sjson, "UsePassives", ui->cbPassivePMsTakenAccount);
    }
  if (ui->sbPMsignalIndividual->value() == 0) on_sbPMsignalIndividual_valueChanged(0);
  else ui->sbPMsignalIndividual->setValue(0);

  flag = FiltJson.contains("ReconstructedEnergy"); //reconstructed energy
  ui->cbActivateEnergyFilter->setChecked(flag);
  if (flag)
    {
      QJsonObject rejson = FiltJson["ReconstructedEnergy"].toObject();
      JsonToCheckbox(rejson, "Active", ui->cbActivateEnergyFilter);
      JsonToLineEdit(rejson, "Min", ui->ledEnergyMin);
      JsonToLineEdit(rejson, "Max", ui->ledEnergyMax);
    }

  flag = FiltJson.contains("LoadedEnergy"); //loaded energy
  ui->cbActivateLoadedEnergyFilter->setChecked(flag);
  if (flag)
    {
      QJsonObject lejson = FiltJson["LoadedEnergy"].toObject();
      JsonToCheckbox(lejson, "Active", ui->cbActivateLoadedEnergyFilter);
      JsonToLineEdit(lejson, "Min", ui->ledLoadEnergyMin);
      JsonToLineEdit(lejson, "Max", ui->ledLoadEnergyMax);
    }

  flag = FiltJson.contains("Chi2"); //chi2
  ui->cbActivateChi2Filter->setChecked(flag);
  if (flag)
    {
      QJsonObject cjson = FiltJson["Chi2"].toObject();
      JsonToCheckbox(cjson, "Active", ui->cbActivateChi2Filter);
      JsonToLineEdit(cjson, "Min", ui->ledChi2Min);
      JsonToLineEdit(cjson, "Max", ui->ledChi2Max);
    }

  ui->cbSpF_LimitToObject->setChecked(false);
  if ( FiltJson.contains("SpatialToPrimary") ) //spatial - primary scint - VERY OLD system
  {
    ui->cbSpF_LimitToObject->setChecked(true);
    ui->leSpF_LimitToObject->setText("PrScint");
  }
  if ( FiltJson.contains("SpatialToObject") )
  {
      ui->cbSpF_LimitToObject->setChecked(true); //compatibility
      QJsonObject jj = FiltJson["SpatialToObject"].toObject();
      JsonToCheckbox(jj, "Active", ui->cbSpF_LimitToObject);
      ui->leSpF_LimitToObject->setText(jj["LimitObject"].toString());
  }

  flag = FiltJson.contains("SpatialCustom"); //spatial - custom
  ui->cbSpFcustom->setChecked(flag);
  if (flag)
    {      
      QJsonObject spfCjson = FiltJson["SpatialCustom"].toObject();
      JsonToCheckbox(spfCjson, "Active", ui->cbSpFcustom);
      ui->cobSpFRecScan->setCurrentIndex(0); //compatibility
      JsonToComboBox(spfCjson, "RecOrScan", ui->cobSpFRecScan);
      int shape = 0;
      parseJson(spfCjson, "Shape", shape);
      JsonToComboBox(spfCjson, "Shape", ui->cobSpFXY);
      JsonToLineEdit(spfCjson, "SizeX", ui->ledSpFsizeX);
      JsonToLineEdit(spfCjson, "SizeY", ui->ledSpFsizeY);
      JsonToLineEdit(spfCjson, "Diameter", ui->ledSpFdiameter);
      JsonToLineEdit(spfCjson, "Side", ui->ledSpFside);
      JsonToLineEdit(spfCjson, "Angle", ui->ledSpFangle);
      JsonToCheckbox(spfCjson, "AllZ", ui->cbSpFallZ);
      JsonToLineEdit(spfCjson, "Zfrom", ui->ledSpFfromZ);
      JsonToLineEdit(spfCjson, "Zto", ui->ledSpFtoZ);
      ui->ledSpF_X0->setText("0"); //compatibility
      JsonToLineEdit(spfCjson, "X0", ui->ledSpF_X0);
      ui->ledSpF_Y0->setText("0"); //compatibility
      JsonToLineEdit(spfCjson, "Y0", ui->ledSpF_Y0);
      ui->cobSpF_cutOutsideInside->setCurrentIndex(0); //compatibility
      JsonToComboBox(spfCjson, "CutOutsideInside", ui->cobSpF_cutOutsideInside);
      if (spfCjson.contains("Polygon"))
        {
          polygon.clear();
          QJsonArray polyarr = spfCjson["Polygon"].toArray();
          for (int i=0; i<polyarr.size(); i++)
            {
              double x = polyarr[i].toArray()[0].toDouble();
              double y = polyarr[i].toArray()[1].toDouble();
              polygon.append(QPointF(x, y));
            }
          if (shape==3 && polygon.size()<3)
            {
              message("Error loading polygon for custom spatial filter", this);
              fOK = false;
            }
        }          
    }

  flag = FiltJson.contains("Correlation"); //correlation
  ui->cbActivateCorrelationFilters->setChecked(flag);
  if (flag)
    {
      QJsonObject corjson = FiltJson["Correlation"].toObject();
      parseJson(corjson, "Active", flag); //if Active is set to false, it will override true to false
      ui->cbActivateCorrelationFilters->setChecked(flag);
      if (flag && corjson.contains("Set"))
        {
          for (int i=0; i<CorrelationFilters.size(); i++) delete CorrelationFilters[i];
          CorrelationFilters.clear();
          QJsonArray jsonArray = corjson["Set"].toArray();
          if (jsonArray.isEmpty())
            {
              message("Correlation filters active, but json does not contain their config", this);
              fOK = false;
            }
          for (int i=0; i<jsonArray.size(); i++)
            {
              QJsonObject js = jsonArray[i].toObject();
              CorrelationFilterStructure* tmp = CorrelationFilterStructure::createFromJson(js, PMgroups, PMgroups->getCurrentGroup(), EventsDataHub);
              if (tmp) CorrelationFilters.append(tmp);
              else
                {
                  message("Error in json config of correlation filter! Number is json array: "+QString::number(i), this);
                  fOK = false;
                }              
            }
        }
      if (ui->sbCorrFNumber->value() == 0) on_sbCorrFNumber_valueChanged(0);
      else ui->sbCorrFNumber->setValue(0);
      ui->pbCorrAccept->setIcon(QIcon());
    }

  //kNN
#ifdef ANTS_FLANN
  flag = FiltJson.contains("kNN");
  ui->cbActivateNNfilter->setChecked(flag);
  if (flag)
    {
      QJsonObject knnjson = FiltJson["kNN"].toObject();
      JsonToCheckbox(knnjson, "Active", ui->cbActivateNNfilter);
      JsonToLineEdit(knnjson, "Min", ui->ledNNmin);
      JsonToLineEdit(knnjson, "Max", ui->ledNNmax);
      JsonToSpinBox(knnjson, "AverageOver", ui->sbNNaverageOver);
    }
#else
  ui->cbActivateNNfilter->setChecked(false);
#endif

  ReconstructionWindow::updateFiltersGui();
  TMPignore = fIgnore; //--//

  return fOK;
}

void ReconstructionWindow::on_pbReconstructAll_clicked()
{
  ReconstructionWindow::writeToJson(MW->Config->JSON);
  ReconstructAll(true);
}

void ReconstructionWindow::ConfigurePlotXY(int binsX, double X0, double X1, int binsY, double Y0, double Y1)
{
    bool fSym = (binsX==binsY && X0==-X0 && Y0==-Y1 && X0==Y0);
    ui->cbXYsymmetric->setChecked(fSym);

    ui->sbXbins->setValue(binsX);
    ui->sbYbins->setValue(binsY);
    ui->ledXfrom->setText(QString::number(X0));
    ui->ledYfrom->setText(QString::number(Y0));
    ui->ledXto->setText(QString::number(X1));
    ui->ledYto->setText(QString::number(Y1));
}

void ReconstructionWindow::ReconstructAll(bool fShow)
{
  MW->fStartedFromGUI = true;
  MW->WindowNavigator->BusyOn();
  ui->pbStopReconstruction->setEnabled(true);
  qApp->processEvents();

  MW->ReconstructionManager->reconstructAll(MW->Config->JSON, MW->GlobSet->NumThreads, fShow);
  //will generate signal when finished and trigger onReconstructionFinished(bool fSuccess)
}

void ReconstructionWindow::onReconstructionFinished(bool fSuccess, bool fShow)
{
  if (fSuccess)
    {
       if (fShow)
           //ShowReconstructionPositionsIfWindowVisible();
           ShowPositions(0, true);
       MW->Owindow->RefreshData();   // *** !!!
       ShowStatistics(true);
       double usPerEvent = ReconstructionManager->getUsPerEvent();
       ui->leoMsPerEv->setText(QString::number(usPerEvent, 'g', 4));
    }
  else
    {
      QString err = ReconstructionManager->getErrorString();
      if (err != "") message(err, this); //skip empty message for user interrupted
      OnEventsDataAdded(); //recon was already cleaned in manager
      ui->leoMsPerEv->setText("");
    }

  if (MW->fStartedFromGUI) SetGuiBusyStatusOff();
  SetProgress(100);
  MW->fStartedFromGUI = false;
}

void ReconstructionWindow::SetShowFirst(bool fOn, int number)
{
  ui->cbLimitNumberEvents->setChecked(fOn);
  if (number != -1) ui->sbLimitNumberEvents->setValue(number);
}

void ReconstructionWindow::on_cbCustomBinning_toggled(bool checked)
{
   ui->fCustomBins->setEnabled(checked);
   if (!checked)
   {
       ui->cbCustomRanges->setChecked(false);
       ui->fCustomRanges->setEnabled(false);
   }
}

void ReconstructionWindow::on_cbCustomRanges_toggled(bool checked)
{
   if (checked) ui->fCustomBins->setEnabled(true);
   ui->fCustomRanges->setEnabled(checked);
   if (checked) ui->cbCustomBinning->setChecked(true);
}

void linkCustomClass::updateBins() {bins = Qbins->value();}

void linkCustomClass::updateRanges() {from = Qfrom->text().toDouble(); to = Qto->text().toDouble();}

void TreeViewStruct::fillCustom(bool fCustomBins, bool fCustomRanges, QVector<linkCustomClass> &cus)
{
  this->fCustomBins = fCustomBins;
  this->fCustomRanges = fCustomRanges;
  if (cus.size()<3) return;

  for (int i=0; i<3; i++)
    {
      Bins[i]=cus[i].bins;
      From[i]=cus[i].from;
      To[i]=cus[i].to;
    }
}

void ReconstructionWindow::on_cbForceCoGgiveZof_toggled(bool /*checked*/)
{
   ui->fCoGStretchZ->setEnabled( ui->cb3Dreconstruction->isChecked() && !ui->cbForceCoGgiveZof->isChecked() );
}

bool ReconstructionWindow::isReconstructEnergy()
{
  return ui->cbReconstructEnergy->isChecked();
}

bool ReconstructionWindow::isReconstructZ()
{
  return ui->cb3Dreconstruction->isChecked();
}

double ReconstructionWindow::getSuggestedEnergy()
{
  return ui->ledSuggestedEnergy->text().toDouble();
}

double ReconstructionWindow::getSuggestedZ()
{
  return ui->ledSuggestedZ->text().toDouble();
}

void ReconstructionWindow::on_cobInRecShape_currentIndexChanged(int index)
{
   ui->fInRecSize2->setEnabled(index == 0);
   ui->lInRecX->setText( (index==0)?"Size X:":"Radius:" );
}

void ReconstructionWindow::on_pbCopyFromRec_clicked()
{
    ui->cbCustomBinning->setChecked(true);
    ui->cbCustomRanges->setChecked(true);

    ui->sbBins1->setValue(ui->sbXbins->value());
    ui->sbBins2->setValue(ui->sbYbins->value());

    ui->ledCustomFrom1->setText(ui->ledXfrom->text());
    ui->ledCustomTo1->setText(ui->ledXto->text());
    ui->ledCustomFrom2->setText(ui->ledYfrom->text());
    ui->ledCustomTo2->setText(ui->ledYto->text());
}

void ReconstructionWindow::on_pbAnalyzeChanPerPhEl_clicked()
{
  if (EventsDataHub->isEmpty()) return;
  if (PMgroups->countPMgroups()>1)
    {
      message("This procedure is implemented only for the case of one PM group");
      return;
    }
  if (!EventsDataHub->isReconstructionReady())
    {
      message("Reconstruction is not ready!", this);
      return;
    }
  if (!Detector->LRFs->isAllLRFsDefined())
    {
      message("LRFs are not ready!", this);
      return;
    }

  SetProgress(0);
  MW->WindowNavigator->BusyOn();
  qApp->processEvents();

  const int numPMs = MW->Detector->PMs->count();
  const double minRange = ui->ledMinRangeChanPerPhEl->text().toDouble();
  const double minRange2 = minRange * minRange;
  const double maxRange = ui->ledMaxRangeChanPerPhEl->text().toDouble();
  const double maxRange2 = maxRange * maxRange;
  MW->TmpHub->ClearTmpHistsSigma2();
  QVector<TH1D*> numberHists; //will be used to calculate sigma vs distance to PM center

  for (int ipm=0; ipm<numPMs; ipm++)
    {
      TString s = "sigma";
      s += ipm;
      TString n = "num";
      n += ipm;
      TH1D* tmp1 = new TH1D(s, "sigma", ui->pbBinsChansPerPhEl->value(), 0,0);

      MW->TmpHub->SigmaHists.append(tmp1);
      tmp1->GetXaxis()->SetTitle("Average signal");
      tmp1->GetYaxis()->SetTitle("Sigma square");
      TH1D* tmp2 = new TH1D(n, "number", ui->pbBinsChansPerPhEl->value(), 0,0); //it seems both  hists update axis synchronously. Same result if fix max range
      numberHists.append(tmp2);
    }

  //filling hists - go through all events, each will populate data for all PMs
  for (int iev=0; iev<EventsDataHub->Events.size(); iev++)
    {
      if (iev % 1000 == 0)
        {
          int ipr = 100.0*iev/EventsDataHub->Events.size();
          SetProgress(ipr);
        }
      if (!EventsDataHub->ReconstructionData[0][iev]->GoodEvent) continue; //respecting the filters
      if (EventsDataHub->ReconstructionData[0][iev]->Points.size()>1) continue; //ignoring multiple events in reconstruction

      const double& x = EventsDataHub->ReconstructionData.at(0).at(iev)->Points[0].r[0];
      const double& y = EventsDataHub->ReconstructionData.at(0).at(iev)->Points[0].r[1];
      double e = EventsDataHub->ReconstructionData.at(0).at(iev)->Points[0].energy;
      if (e < 1e-10) e = 1.0;
      for (int ipm=0; ipm<numPMs; ipm++)
        {
           double x0 = MW->Detector->PMs->X(ipm);
           double y0 = MW->Detector->PMs->Y(ipm);
           double r2 = (x-x0)*(x-x0) + (y-y0)*(y-y0);
           if ( r2 < minRange2  ||  r2 > maxRange2 ) continue;

           double AvSig = Detector->LRFs->getLRF(ipm, EventsDataHub->ReconstructionData.at(0).at(iev)->Points[0].r);
           double sig = EventsDataHub->Events[iev][ipm];
           double delta2 = sig/e - AvSig; //take energy into account
           delta2 *= delta2;

           MW->TmpHub->SigmaHists[ipm]->Fill(AvSig, delta2);
           numberHists[ipm]->Fill(AvSig, 1.0);
        }
    }

  //calculating sigmas
  const int MinEntries = 5;
  for (int ipm=0; ipm<numPMs; ipm++)
    {
      for (int i=1; i<numberHists[ipm]->GetXaxis()->GetNbins()-1; i++) //ignoring under and over bins
        {
          if (numberHists[ipm]->GetBinContent(i) < MinEntries) MW->TmpHub->SigmaHists[ipm]->SetBinContent(i, 0);
          else MW->TmpHub->SigmaHists[ipm]->SetBinContent(i, MW->TmpHub->SigmaHists[ipm]->GetBinContent(i)/numberHists[ipm]->GetBinContent(i));
        }
    }

  for (int ipm=0; ipm<numberHists.size(); ipm++) delete numberHists[ipm];
  numberHists.clear();

  SetProgress(100);
  MW->WindowNavigator->BusyOff(false);
}

void ReconstructionWindow::on_pbChanPerPhElShow_clicked()
{
  int ipm = ui->sbChanPerPhElPM->value();
  if (ipm >= MW->TmpHub->SigmaHists.size())
    {
      message("Wrong PM number or data not ready!", this);
      return;
    }
  MW->GraphWindow->Draw(MW->TmpHub->SigmaHists[ipm], "", true, false);
}

void ReconstructionWindow::on_sbChanPerPhElPM_valueChanged(int arg1)
{
   if (arg1> MW->Detector->PMs->count()-1)
     {
       ui->sbChanPerPhElPM->setValue(0);
       return;
     }

   if (MW->TmpHub->ChPerPhEl_Sigma2.isEmpty())
     {
       ui->ledChansPerPhEl->setText("");
       return;
     }
   ui->ledChansPerPhEl->setText( QString::number(MW->TmpHub->ChPerPhEl_Sigma2[arg1], 'g', 3) );
   if (MW->GraphWindow->isVisible()) on_pbChanPerPhElShow_clicked();
}

void ReconstructionWindow::on_pbExtractChansPerPhEl_clicked()
{
  int ipm = ui->sbChanPerPhElPM->value();
  int numPMs = MW->Detector->PMs->count();

  if (MW->TmpHub->SigmaHists.size() != numPMs)
    {
      message("There is no data - use 'Prepare data...' first!", this);
      return;
    }

  if (MW->TmpHub->ChPerPhEl_Sigma2.size()<numPMs) MW->TmpHub->ChPerPhEl_Sigma2.resize(numPMs);

  if (ui->cbExtractChPerPhElForAll->isChecked())
    {
      MW->Owindow->OutText("---------");
      MW->Owindow->OutText("#PM   Channels_per_photoelectron");
      //doing for all PMs
      for (int ipm=0; ipm<numPMs; ipm++)
        {
          double ChanelsPerPhEl = extractChPerPHEl(ipm);
          if (ChanelsPerPhEl < 0)
          {
              message("Fit failed!", this);
              return;
          }
          MW->TmpHub->ChPerPhEl_Sigma2[ipm] = ChanelsPerPhEl;
          QString txt = QString::number(ipm)+" "+QString::number(MW->TmpHub->ChPerPhEl_Sigma2[ipm], 'g', 3);
          if (MW->TmpHub->ChPerPhEl_Peaks.size() == MW->TmpHub->ChPerPhEl_Sigma2.size())
              txt += "  from peaks: "+QString::number(MW->TmpHub->ChPerPhEl_Peaks[ipm], 'g', 3);
          MW->Owindow->OutText(txt);
        }
    }
  else
    {
      //doing only for one selected PM
      if (ipm > MW->TmpHub->SigmaHists.size()-1)
        {
          message("Wrong PM number or data not ready!", this);
          return;
        }

      MW->GraphWindow->Draw(MW->TmpHub->SigmaHists[ipm], "", true, false);
      double ChanelsPerPhEl = extractChPerPHEl(ipm);
      if (ChanelsPerPhEl < 0)
      {
          message("Fit failed!", this);
          return;
      }
      MW->GraphWindow->UpdateRootCanvas();

      MW->TmpHub->ChPerPhEl_Sigma2[ipm] = ChanelsPerPhEl;
    }

  if (ipm<MW->TmpHub->ChPerPhEl_Sigma2.size()) ui->ledChansPerPhEl->setText( QString::number(MW->TmpHub->ChPerPhEl_Sigma2[ipm], 'g', 3) );
  else ui->ledChansPerPhEl->setText("");
}

double ReconstructionWindow::extractChPerPHEl(int ipm)
{
  int numPMs = MW->Detector->PMs->count();
  if (ipm<0 || ipm>numPMs-1) return -1;

  double upperLim = ui->ledUpperLimitForChanPerPhEl->text().toDouble();
  double lowerLim = ui->ledLowerLimitForChanPerPhEl->text().toDouble();
  TFitResultPtr fit = MW->TmpHub->SigmaHists[ipm]->Fit("pol1", "S", "", lowerLim, upperLim);

  if (!fit.Get())
    {
      qWarning() << "Fit failed!";
      return -1;
    }

  double cutoff = fit->Value(0);
  double ChannelsPerPhEl = fit->Value(1);
  double ENF = ui->ledENFforChanPerPhEl->text().toDouble();
  if (ENF <= 0)
    {
      qWarning() << "ENF is zero or negative, ignoring!";
    }
  else ChannelsPerPhEl /= ENF;
  qDebug() << ipm << "> ChPerPhEl:"<<ChannelsPerPhEl<<" const:"<<cutoff;

  return ChannelsPerPhEl;
}

bool ReconstructionWindow::IntersectionOfTwoLines(double A1, double B1, double C1, double A2, double B2, double C2, double *result)
{
    double det = A1*B2 - A2*B1;

    if (fabs(det) < 1.0e-10)
      {
         //lines are ~parallel
        return false;
      }
    else
      {
        result[0] = (B2*C1 - B1*C2)/det;
        result[1] = (A1*C2 - A2*C1)/det;
      }

    return true;
}

void ReconstructionWindow::on_cobSpFXY_currentIndexChanged(int index)
{
  ui->fSpF_Origin->setVisible(index!=3);
}

void ReconstructionWindow::on_cbReconstructEnergy_toggled(bool checked)
{
   ui->lReconstructEnergy->setText( checked ? "Initial energy:" : "Fixed energy:" );
}

void ReconstructionWindow::on_cobZ_currentIndexChanged(int index)
{
   QPalette p = ui->cobZ->palette();
   if (index == 0)
     {
       ui->ledSuggestedZ->setVisible(true);
       ui->label_8->setVisible(true);
       ui->cobZ->setGeometry(ui->cobZ->x(), ui->cobZ->y(), 131, ui->cobZ->height());
       // combo button
       p.setColor(QPalette::Button, Qt::white);
       p.setColor(QPalette::ButtonText, Qt::black);
       // combo menu
       //p.setColor(QPalette::Base, Qt::white);
       //p.setColor(QPalette::Text, Qt::red);
     }
   else
     {
       ui->ledSuggestedZ->setVisible(false);
       ui->label_8->setVisible(false);
       ui->cobZ->setGeometry(ui->cobZ->x(), ui->cobZ->y(), 188, ui->cobZ->height());
       p.setColor(QPalette::Button, Qt::white);
       p.setColor(QPalette::ButtonText, Qt::red);
     }
   ui->cobZ->setPalette(p);

   if (ui->cb3Dreconstruction->isChecked())
     {
       //3D - reconstruct Z
       ui->cobZ->setItemText(0, "Start from Z of:");
       ui->cobZ->setItemText(1, "Start from loaded/sim Z");
       //if (ui->cobZ->count() == 3) ui->cobZ->removeItem(2);
     }
   else
     {
       //2D
       ui->cobZ->setItemText(0, "Set Z to:");
       ui->cobZ->setItemText(1, "Use loaded/sim Z");
       //if (ui->cobZ->count() == 2) ui->cobZ->addItem("");
       //ui->cobZ->setItemText(2, "Do not change Z");
     }
}

void ReconstructionWindow::on_cbLimitNumberEvents_toggled(bool checked)
{
  if (checked) ui->lShowFirst->setText("<font color='red'>first</font>");
  else ui->lShowFirst->setText("first");
}

void ReconstructionWindow::on_sbXbins_editingFinished()
{
  if (ui->cbXYsymmetric->isChecked())
      ui->sbYbins->setValue(ui->sbXbins->value());
}

void ReconstructionWindow::on_cbXYsymmetric_toggled(bool checked)
{
  //disable is in signal/slot gui
  if (checked)
    {
      QString txt = ui->ledXto->text();
      ui->ledXfrom->setText("-"+txt);
      ui->ledYfrom->setText("-"+txt);
      ui->ledYto->setText(txt);
      ui->sbYbins->setValue(ui->sbXbins->value());
    }
}

void ReconstructionWindow::on_ledXto_editingFinished()
{
  if (ui->cbXYsymmetric->isChecked())
    {
      QString txt = ui->ledXto->text();
      ui->ledXfrom->setText("-"+txt);
      ui->ledYfrom->setText("-"+txt);
      ui->ledYto->setText(txt);
    }
}

void ReconstructionWindow::on_pbTrueToRec_clicked()
{
  EventsDataHub->copyTrueToReconstructed();
  SetProgress(100);
  ShowPositions(0, true);
  ShowStatistics();
  ui->leoMsPerEv->setText("");
}

void ReconstructionWindow::on_pbRecToTrue_clicked()
{
  EventsDataHub->copyReconstructedToTrue();
  ui->leoMsPerEv->setText("");
}

void ReconstructionWindow::on_pbTrueScanToManifest_clicked()
{
  if (EventsDataHub->isScanEmpty()) return;

  double d = ui->ledDiameterForTrueTomanifest->text().toDouble();
  EventsDataHub->clearManifest();

  for (int i=0; i<EventsDataHub->Scan.size(); i++)
    {
      double x = EventsDataHub->Scan.at(i)->Points[0].r[0];
      double y = EventsDataHub->Scan.at(i)->Points[0].r[1];

      if (i>0)
        if (x==EventsDataHub->Manifest.last()->X && y==EventsDataHub->Manifest.last()->Y)
          continue; //already have one of that

      ManifestItemHoleClass* m = new  ManifestItemHoleClass(d);
      m->X = x;
      m->Y = y;
      EventsDataHub->Manifest.append(m);
    }
  ui->cbShowManifestItems->setEnabled(true);
}

void ReconstructionWindow::on_pbManifestLineAttributes_clicked()
{
  if (EventsDataHub->Manifest.isEmpty()) return;

  int color = EventsDataHub->Manifest.first()->LineColor;
  int style = EventsDataHub->Manifest.first()->LineStyle;
  int width = EventsDataHub->Manifest.first()->LineWidth;

  ARootLineConfigurator* rlc = new ARootLineConfigurator(&color, &width, &style, this);
  int res = rlc->exec();
  if (res != 0)
  {
      for (int i=0; i<EventsDataHub->Manifest.size(); i++)
        {
          EventsDataHub->Manifest[i]->LineColor = color;
          EventsDataHub->Manifest[i]->LineWidth = width;
          EventsDataHub->Manifest[i]->LineStyle = style;
        }
  }
  delete rlc;
}

void ReconstructionWindow::on_pbShiftManifested_clicked()
{   
   double dx = ui->ledDeltaXmanifested->text().toDouble();
   double dy = ui->ledDeltaYmanifested->text().toDouble();
   for (int i=0; i<EventsDataHub->Manifest.size(); i++)
     {
       EventsDataHub->Manifest[i]->X += dx;
       EventsDataHub->Manifest[i]->Y += dy;
     }
}

void ReconstructionWindow::on_pbRotateManifested_clicked()
{
  double angle = ui->ledDeltaAnglemanifested->text().toDouble()*3.1415926535/180.0;

  double cosA = cos(angle);
  double sinA = sin(angle);

  for (int i=0; i<EventsDataHub->Manifest.size(); i++)
    {
      double newX = EventsDataHub->Manifest[i]->X * cosA  -  EventsDataHub->Manifest[i]->Y * sinA;
      double newY = EventsDataHub->Manifest[i]->X * sinA  +  EventsDataHub->Manifest[i]->Y * cosA;
      EventsDataHub->Manifest[i]->X = newX;
      EventsDataHub->Manifest[i]->Y = newY;
    }
}

void ReconstructionWindow::on_pbDeclareNumRuns_clicked()
{
   EventsDataHub->ScanNumberOfRuns = ui->sbDeclareNumRuns->value();
}

void ReconstructionWindow::on_pbAddValueToAllSignals_clicked()
{
    double val = ui->ledAddValueToAllSignals->text().toDouble();

    for (int i=0; i<EventsDataHub->Events.size(); i++)
      {
        for (int j=0; j<MW->Detector->PMs->count(); j++)
          {
            EventsDataHub->Events[i][j] += val;
          }
      }

    message(ui->ledAddValueToAllSignals->text()+" was added to signal values for all events and sensors");
}

void ReconstructionWindow::on_pbPMTcol_clicked()
{
  int col = PMcolor;
  int wid = PMwidth;
  int styl = PMstyle;
  ARootLineConfigurator* rlc = new ARootLineConfigurator(&col, &wid, &styl, this);
  int res = rlc->exec();
  if (res != 0)
  {
      PMcolor = col;
      PMwidth = wid;
      PMstyle = styl;
  }
  delete rlc;
}

void ReconstructionWindow::on_pbUpdateReconConfig_clicked()
{
    UpdateReconConfig();
}

void ReconstructionWindow::UpdateReconConfig()
{
    ReconstructionWindow::writeToJson(MW->Config->JSON);
    if (MW->GenScriptWindow)
        if (MW->GenScriptWindow->isVisible())
            MW->GenScriptWindow->updateJsonTree();    
}

void ReconstructionWindow::on_cobCurrentGroup_activated(int index)
{
      //qDebug() << "Current group index clicked:"<<index;
    if (index < 0) return;

    if (index > PMgroups->countPMgroups()-1 )
    {
        message("Error: mismatch in current group: GUI vs manager", this);
        return;
    }
    PMgroups->setCurrentGroup(index);
    onRequestReconstructionGuiUpdate();
}

void ReconstructionWindow::on_pbConfigureGroups_toggled(bool checked)
{
    ui->swRecOrGroups->setCurrentIndex( checked ? 1 : 0);
}

void ReconstructionWindow::on_pbBackToRecon_clicked()
{
   ui->pbConfigureGroups->setChecked(false);
}

void ReconstructionWindow::on_lwPMgroups_customContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem* thisItem = ui->lwPMgroups->itemAt(pos);
    if (!thisItem) return;

    QMenu myMenu;
    myMenu.addSeparator();
    QAction* Show = myMenu.addAction("Show");
    myMenu.addSeparator();
    QAction* Rename = myMenu.addAction("Rename");
    myMenu.addSeparator();
    QAction* Remove = myMenu.addAction("Remove");
    myMenu.addSeparator();

    QPoint globalPos = ui->lwPMgroups->mapToGlobal(pos);
    QAction* selectedItem = myMenu.exec(globalPos);

    int row = ui->lwPMgroups->currentRow();
    if (selectedItem == Show)
        showSensorGroup(row);
    if (selectedItem == Rename)
    {
        QString newName = QInputDialog::getText(this, "Rename sensor group", "Enter new name:", QLineEdit::Normal,  MW->Detector->PMgroups->getGroupName(row));
        MW->Detector->PMgroups->renameGroup(row, newName);
        onRequestReconstructionGuiUpdate();
    }
    if (selectedItem == Remove)
    {
        MW->Detector->PMgroups->removeGroup(row);  //also does json update
        onRequestReconstructionGuiUpdate();
    }   
}

void ReconstructionWindow::on_pbShowUnassigned_clicked()
{
   QVector<int> Unassigned;
   if (PMgroups->countPMsWithoutGroup(&Unassigned)==0)
   {
       message("All PMs are assigned to groups", this);
       return;
   }

   QVector<QString> tmp;
   for (int i=0; i<PMs->count(); i++)
   {
       if (Unassigned.contains(i)) tmp.append(QString::number(i));
       else tmp.append("");
   }
   MW->GeometryWindow->show();
   MW->GeometryWindow->raise();
   MW->GeometryWindow->activateWindow();
   MW->GeometryWindow->ShowTextOnPMs(tmp, kBlack);
}

void ReconstructionWindow::on_lwPMgroups_activated(const QModelIndex &index)
{
    int igroup = index.row();
    if (igroup<ui->cobAssignToPMgroup->count())
        ui->cobAssignToPMgroup->setCurrentIndex(igroup);
}

void ReconstructionWindow::on_lwPMgroups_clicked(const QModelIndex &index)
{
    int igroup = index.row();
    if (igroup<ui->cobAssignToPMgroup->count())
        ui->cobAssignToPMgroup->setCurrentIndex(igroup);
}

void ReconstructionWindow::on_cobLRFmodule_currentIndexChanged(int index)
{
    bool fOld = (index==0);

    if (fOld) Detector->LRFs->selectOld();
    else Detector->LRFs->selectNew();
    ui->labLRFmoduleWarning->setVisible( !fOld );

    ReconstructionWindow::LRF_ModuleReadySlot(false); //the selector will ask the status!
}

void ReconstructionWindow::on_pbShowLRFwindow_clicked()
{
  if (Detector->LRFs->isOldSelected())
    {
      //Old module
      MW->lrfwindow->showNormal();
      MW->lrfwindow->raise();
      MW->lrfwindow->activateWindow();
    }
  else
    {
      //New module
      MW->newLrfWindow->showNormal();
      MW->newLrfWindow->raise();
      MW->newLrfWindow->activateWindow();
    }
}

void ReconstructionWindow::on_pbLoadLRFs_clicked()
{
  if (Detector->LRFs->isOldSelected())
    {
       //Old module
       MW->lrfwindow->LoadLRFDialog(this);
    }
  else
    {
       //New module
       //
    }
}

void ReconstructionWindow::on_pbSaveLRFs_clicked()
{
  if (Detector->LRFs->isOldSelected())
    {
       //Old module
       MW->lrfwindow->SaveLRFDialog(this);
    }
  else
    {
       //New module
       //
    }
}

void ReconstructionWindow::on_sbShowEventsEvery_valueChanged(int)
{
    if (MW->GraphWindow->LastDistributionShown == "ShowVsXY")
      ReconstructionWindow::on_pbShowVsXY_clicked();
}

void ReconstructionWindow::on_lwPMgroups_doubleClicked(const QModelIndex &/*index*/)
{
    if (!MW->GeometryWindow->isVisible()) return;
    int igroup = ui->lwPMgroups->currentRow();
    showSensorGroup(igroup);
}

void ReconstructionWindow::on_pbShowSensorGroup_clicked()
{
    int igroup = ui->cobAssignToPMgroup->currentIndex();
    showSensorGroup(igroup);
}

void ReconstructionWindow::showSensorGroup(int igroup)
{
    QVector<QString> tmp;
    for (int i=0; i<PMs->count(); i++)
    {
        QString s;
        if (PMgroups->isPmBelongsToGroup(i, igroup))
             s = QString::number( igroup );
        else s = "";
        tmp.append(s);
    }
    MW->GeometryWindow->show();
    MW->GeometryWindow->raise();
    MW->GeometryWindow->activateWindow();
    MW->GeometryWindow->ShowTextOnPMs(tmp, kRed);
}

void ReconstructionWindow::on_leSpF_LimitToObject_textChanged(const QString &/*arg1*/)
{
    bool fFound = (ui->cbSpF_LimitToObject->isChecked()) ?
         Detector->Sandwich->isVolumeExist(ui->leSpF_LimitToObject->text()) : true;

    QPalette palette = ui->leSpF_LimitToObject->palette();
    palette.setColor(QPalette::Text, (fFound ? Qt::black : Qt::red) );
    ui->leSpF_LimitToObject->setPalette(palette);
}

void ReconstructionWindow::on_cbSpF_LimitToObject_toggled(bool /*checked*/)
{
    on_leSpF_LimitToObject_textChanged("");
}

void ReconstructionWindow::on_pbUpdateGuiSettingsInJSON_clicked()
{
    updateGUIsettingsInConfig();
}

static Int_t npeaks = 10;
Double_t fpeaks(Double_t *x, Double_t *par)
{
   Double_t result = par[0] + par[1]*x[0];
   for (Int_t p=0;p<npeaks;p++) {
      Double_t norm  = par[3*p+2];
      Double_t mean  = par[3*p+3];
      Double_t sigma = par[3*p+4];
      result += norm*TMath::Gaus(x[0],mean,sigma);
   }
   return result;
}

void ReconstructionWindow::on_pbPrepareSignalHistograms_clicked()
{
    if (EventsDataHub->isEmpty())
      {
        message("There are no signal data!", this);
        return;
      }

    MW->TmpHub->ClearTmpHistsPeaks();
    MW->TmpHub->ChPerPhEl_Sigma2.clear();

    for (int ipm=0; ipm<PMs->count(); ipm++)
    {
        TH1D* h = new TH1D("", "", ui->sbFromPeaksBins->value(), ui->ledFromPeaksFrom->text().toDouble(),ui->ledFromPeaksTo->text().toDouble());
        h->SetXTitle("Signal");

      for (int iev = 0; iev < EventsDataHub->Events.size(); iev++)
            h->Fill(EventsDataHub->getEvent(iev)->at(ipm));

      MW->TmpHub->PeakHists << h;
      if (ipm==0) MW->GraphWindow->Draw(h, "", true, false);
    }
}

#include "apeakfinder.h"
#include "TGraph.h"
#include "TLine.h"

void ReconstructionWindow::on_pbFrindPeaks_clicked()
{
  if (MW->TmpHub->PeakHists.size() != PMs->count())
  {
      message("Signal data not ready!", this);
      return;
  }

  MW->TmpHub->FoundPeaks.clear();
  MW->TmpHub->ChPerPhEl_Peaks.clear();

  QVector<int> failedPMs;
  for (int ipm=0; ipm<PMs->count(); ipm++)
  {
      APeakFinder f(MW->TmpHub->PeakHists.at(ipm));
      QVector<double> peaks = f.findPeaks(ui->ledFromPeaksSigma->text().toDouble(), ui->ledFromPeaksThreshold->text().toDouble(), ui->sbFromPeaksMaxPeaks->value(), true);

      std::sort(peaks.begin(), peaks.end());

      MW->TmpHub->FoundPeaks << peaks;

      if (peaks.size() < 2)
      {
          failedPMs << ipm;
          MW->TmpHub->ChPerPhEl_Peaks << -1;
          continue;
      }

      TGraph g;
      for (int i=0; i<peaks.size(); i++)
         g.SetPoint(i, i, peaks.at(i));

      double constant, slope;
      int ifail;
      g.LeastSquareLinearFit(peaks.size(), constant, slope, ifail);
      if ( ifail != 0 )
      {
          failedPMs << ipm;
          MW->TmpHub->ChPerPhEl_Peaks << -1;
          continue;
      }
      else MW->TmpHub->ChPerPhEl_Peaks.append(slope);
    }

  on_pbFromPeaksShow_clicked();
  if (!failedPMs.isEmpty())
  {
      QString s = "Peaks < 2 or failed fit for PM#:";
      for (int i : failedPMs) s += " " + QString::number(i);
      message(s, this);
  }
}

void ReconstructionWindow::on_pbFromPeaksToPreprocessing_clicked()
{
    if (MW->TmpHub->ChPerPhEl_Peaks.size() != PMs->count())
    {
        message("Data not ready", this);
        return;
    }
    MW->SetMultipliersUsingChPhEl(MW->TmpHub->ChPerPhEl_Peaks);
}

void ReconstructionWindow::on_pbFromPeaksPedestals_clicked()
{
  if (MW->TmpHub->FoundPeaks.size() != PMs->count())
  {
      message("Data not ready", this);
      return;
  }
  QVector<double> Pedestals;
  for (int i=0; i<PMs->count(); i++)
    {
      if (MW->TmpHub->FoundPeaks.at(i).isEmpty())
        {
          message(QString("PM# ") + QString::number(i) +  " has no detected peaks!", this);
          return;
        }
      Pedestals << MW->TmpHub->FoundPeaks.at(i).first();
    }

  MW->CorrectPreprocessingAdds(Pedestals);
}

void ReconstructionWindow::on_pbFromPeaksShow_clicked()
{
  int ipm = ui->sbFrompeakPM->value();
  int numPMs = PMs->count();

  if (MW->TmpHub->PeakHists.size() != numPMs) return;
  if (ipm >= MW->TmpHub->PeakHists.size()) return;
  if (!MW->TmpHub->PeakHists.at(ipm)) return;

  TH1D* h = new TH1D( *(MW->TmpHub->PeakHists.at(ipm)) );


  if (!MW->GraphWindow->isVisible()) MW->GraphWindow->showNormal();
  MW->GraphWindow->DrawWithoutFocus(h, "", true, true);

  if (MW->TmpHub->FoundPeaks.size() == numPMs)
    {
      const QVector<double> &peaks = MW->TmpHub->FoundPeaks.at(ipm);
      for (int i=0; i<peaks.size(); i++)
        {
          TLine* l = new TLine(peaks.at(i), -1e10, peaks.at(i), 1e10);
          l->SetLineColor(kRed);
          l->SetLineWidth(2);
          l->SetLineStyle(2);
          MW->GraphWindow->DrawWithoutFocus(l, "same", false, true);
        }
      MW->GraphWindow->ShowTextPanel(QString("Channels per ph.e.: ")+QString::number(MW->TmpHub->ChPerPhEl_Peaks.at(ipm)));
    }
  MW->GraphWindow->UpdateRootCanvas();
  MW->GraphWindow->LastDistributionShown = "SignalPeaks";
}

void ReconstructionWindow::on_sbFrompeakPM_valueChanged(int arg1)
{
    if (arg1 >= PMs->count())
      {
        if (PMs->count() == 0) return;
        ui->sbFrompeakPM->setValue(PMs->count()-1);
        return; //update on_change
      }
    if (MW->GraphWindow->LastDistributionShown == "SignalPeaks") on_pbFromPeaksShow_clicked();
}

void ReconstructionWindow::on_pbClearAllFilters_clicked()
{
    ui->cbFilterEventNumber->setChecked(false);
    ui->cbFilterMultipleScanEvents->setChecked(false);
    ui->cbFilterSumSignal->setChecked(false);
    ui->cbFilterIndividualSignals->setChecked(false);
    ui->cbActivateEnergyFilter->setChecked(false);
    ui->cbActivateLoadedEnergyFilter->setChecked(false);
    ui->cbActivateChi2Filter->setChecked(false);
    ui->cbSpF_LimitToObject->setChecked(false);
    ui->cbSpFcustom->setChecked(false);
    ui->cbActivateCorrelationFilters->setChecked(false);
    ui->cbActivateNNfilter->setChecked(false);

    ReconstructionWindow::on_pbUpdateFilters_clicked();
}
