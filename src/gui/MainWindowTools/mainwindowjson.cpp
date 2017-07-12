#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "detectorclass.h"
#include "materialinspectorwindow.h"
#include "amaterialparticlecolection.h"
#include "detectoraddonswindow.h"
#include "outputwindow.h"
#include "geometrywindowclass.h"
#include "aconfiguration.h"
#include "ajsontools.h"
#include "particlesourcesclass.h"
#include "aparticleonstack.h"
#include "globalsettingsclass.h"
#include "pms.h"
#include "eventsdataclass.h"
#include "tmpobjhubclass.h"
#include "reconstructionwindow.h"

#include <QJsonObject>
#include <QDebug>
#include <QVector3D>

#include "TH1I.h"
#include "TH1D.h"
#include "TGeoManager.h"

// Detector GUI
void MainWindow::writeDetectorToJson(QJsonObject &json)
{
  //qDebug() << "DetGui->Json";
  writeLoadExpDataConfigToJson(Detector->PreprocessingJson);
  Detector->writeToJson(json);
}

// will be obsolete after conversion!
bool MainWindow::readDetectorFromJson(QJsonObject &json)
{
  GeometryWindow->fRecallWindow = false;
  bool fOK = Detector->MakeDetectorFromJson(json);
  if (!fOK)
    {
      qCritical() << "Error while loading detector config";
      return false;
    }

  //gui update is automatically triggered
  return true;
}

void MainWindow::onRequestDetectorGuiUpdate()
{
    //qDebug() << "DetJson->DetGUI";
  Detector->GeoManager->SetNsegments(GlobSet->NumSegments); //lost because GeoManager is new

  DoNotUpdateGeometry = true;
  //Particles
  MainWindow::ListActiveParticles();
  //Materials
  MainWindow::on_pbRefreshMaterials_clicked();
  MIwindow->UpdateActiveMaterials(); //refresh indication on Material Inspector window
  MIwindow->setLogLog(Detector->MpCollection->fLogLogInterpolation);
  //Optical overrides
  MainWindow::on_pbRefreshOverrides_clicked();
  //PM types
  if (ui->sbPMtype->value() > PMs->countPMtypes()-1) ui->sbPMtype->setValue(0);
  MainWindow::updateCOBsWithPMtypeNames();
  MainWindow::on_pbRefreshPMproperties_clicked();
  //PM arrays
  ui->cbUPM->setChecked(Detector->PMarrays[0].fActive);
  ui->cbLPM->setChecked(Detector->PMarrays[1].fActive);
  MainWindow::on_pbShowPMsArrayRegularData_clicked(); //update regular array fields
  //Detector addon window updates
  DAwindow->UpdateGUI();
  if (Detector->PMdummies.size()>0) DAwindow->UpdateDummyPMindication();
  //Output window
  Owindow->RefreshData();
  Owindow->ResetViewport();  // *** !!! extend to other windows too!
  //world
  ui->ledTopSizeXY->setText( QString::number(2.0*Detector->WorldSizeXY, 'g', 4) );
  ui->ledTopSizeZ->setText( QString::number(2.0*Detector->WorldSizeZ, 'g', 4) );
  ui->cbFixedTopSize->setChecked( Detector->fWorldSizeFixed );
  //misc
  MainWindow::ShowPMcount();
  MainWindow::CheckPresenseOfSecScintillator(); //checks if SecScint present, and update selectors of primary/secondary scintillation
  //Sandwich - now automatic
  //PM explore
  MainWindow::on_pbIndPMshowInfo_clicked();
  //Electronics
  MainWindow::on_pbElUpdateIndication_clicked();
  //GDML?
  onGDMLstatusChage(!Detector->isGDMLempty());

  Detector->checkSecScintPresent();
  ui->fSecondaryLightGenType->setEnabled(Detector->fSecScintPresent);

  //readExtraGuiFromJson(Config->JSON); //new!
  //qDebug() << "Before:\n"<<Config->JSON["DetectorConfig"].toObject()["LoadExpDataConfig"].toObject();
  UpdatePreprocessingSettingsIndication();
  //qDebug() << "AFTER:\n"<<Config->JSON["DetectorConfig"].toObject()["LoadExpDataConfig"].toObject();

  DoNotUpdateGeometry = false;

  if (GeometryWindow->isVisible())
    {
      MainWindow::ShowGeometry(false);
      MainWindow::ShowTracks();
    }
}

void MainWindow::onNewConfigLoaded()
{
    //qDebug() << "-->New detector config was loaded";
    initOverridesAfterLoad();

    readExtraGuiFromJson(Config->JSON);
}

void MainWindow::initOverridesAfterLoad()
{
    int iFrom, iTo;
    Detector->MpCollection->getFirstOverridenMaterial(iFrom, iTo);
    ui->cobMaterialForOverrides->setCurrentIndex(iFrom);
    ui->cobMaterialTo->setCurrentIndex(iTo);
    MainWindow::on_pbRefreshOverrides_clicked();
}

   // extra GUI-specific settings
void MainWindow::writeExtraGuiToJson(QJsonObject &json)
{
  QJsonObject jmain;

  QJsonObject jsMW;
  jsMW["ConfigGuiLocked"] = fConfigGuiLocked;
  jmain["MW"] = jsMW;

  QJsonObject js;
  js["Particle"] = ui->cobParticleToStack->currentIndex();
  js["Copies"] = ui->sbNumCopies->value();
  js["X"] = ui->ledParticleStackX->text().toDouble();
  js["Y"] = ui->ledParticleStackY->text().toDouble();
  js["Z"] = ui->ledParticleStackZ->text().toDouble();
  js["dX"] = ui->ledParticleStackVx->text().toDouble();
  js["dY"] = ui->ledParticleStackVy->text().toDouble();
  js["dZ"] = ui->ledParticleStackVz->text().toDouble();
  js["Energy"] = ui->ledParticleStackEnergy->text().toDouble();
  js["Time"] = ui->ledParticleStackTime->text().toDouble();
  js["ScriptEV"] = CheckerScript;
  jmain["PartcleStackChecker"] = js;

  QJsonObject jeom;
  jeom["ZoomLevel"] = GeometryWindow->ZoomLevel;
  jmain["GeometryWindow"] = jeom;

  //Misc setting (PlotXY, blur)
  Rwindow->writeMiscGUIsettingsToJson(jmain);

  json["GUI"] = jmain;
}

void MainWindow::readExtraGuiFromJson(QJsonObject &json)
{
  if (!json.contains("GUI")) return;
  QJsonObject jmain = json["GUI"].toObject();

  fConfigGuiLocked = false;
  if (jmain.contains("MW"))
    {
       QJsonObject jsMW = jmain["MW"].toObject();
       parseJson(jsMW, "ConfigGuiLocked", fConfigGuiLocked);
    }
  onGuiEnableStatus(fConfigGuiLocked);

  QJsonObject js = jmain["PartcleStackChecker"].toObject();
  if (!js.isEmpty())
    {
      JsonToComboBox(js, "Particle", ui->cobParticleToStack);
      JsonToSpinBox(js, "Copies", ui->sbNumCopies);
      JsonToLineEdit(js, "X", ui->ledParticleStackX);
      JsonToLineEdit(js, "Y", ui->ledParticleStackY);
      JsonToLineEdit(js, "Z", ui->ledParticleStackZ);
      JsonToLineEdit(js, "dX", ui->ledParticleStackVx);
      JsonToLineEdit(js, "dY", ui->ledParticleStackVy);
      JsonToLineEdit(js, "dZ", ui->ledParticleStackVz);
      JsonToLineEdit(js, "Energy", ui->ledParticleStackEnergy);
      JsonToLineEdit(js, "Time", ui->ledParticleStackTime);
      parseJson(js, "ScriptEV", CheckerScript);
    }

  js = jmain["GeometryWindow"].toObject();
  if (js.contains("ZoomLevel"))
    {
      GeometryWindow->ZoomLevel = js["ZoomLevel"].toInt();
      GeometryWindow->Zoom(true);
    }
  if (jmain.contains("ReconstructionWindow"))
      Rwindow->readMiscGUIsettingsFromJson(jmain);
}

void MainWindow::writeLoadExpDataConfigToJson(QJsonObject &json)
{
    json["Preprocessing"] = ui->cbPMsignalPreProcessing->isChecked();
      QJsonArray ar;
      for (int ipm=0; ipm<PMs->count(); ipm++)
      {
        QJsonArray el;
        el.append(PMs->at(ipm).PreprocessingAdd);
        el.append(PMs->at(ipm).PreprocessingMultiply);
        ar.append(el);
      }
    json["AddMulti"] = ar;

      QJsonObject enj;
      enj["Activated"] = ui->cbLoadedDataHasEnergy->isChecked();
      enj["Channel"] = ui->sbLoadedEnergyChannelNumber->value();
      enj["Add"] = TmpHub->PreEnAdd;
      enj["Multi"] = TmpHub->PreEnMulti;
    json["LoadedEnergy"] = enj;

      QJsonObject posj;
      posj["Activated"] = ui->cbLoadedDataHasPosition->isChecked();
      posj["Channel"] = ui->sbLoadASCIIpositionXchannel->value();
    json["LoadedPosition"] = posj;

      QJsonObject zposj;
      zposj["Activated"] = ui->cbLoadedDataHasZPosition->isChecked();
      zposj["Channel"] = ui->sbLoadASCIIpositionZchannel->value();
    json["LoadedZPosition"] = zposj;

      QJsonObject ij;
      ij["Activated"] = ui->cbIgnoreEvent->isChecked();
      ij["Min"] = ui->ledIgnoreThresholdMin->text().toDouble();
      ij["Max"] = ui->ledIgnoreThresholdMax->text().toDouble();
    json["IgnoreThresholds"] = ij;

    json["LoadFirst"] = ( ui->cbLimitNumberEvents->isChecked() ? ui->sbFirstNevents->value() : -1 );

    json["ManifestFile"] = ui->leManifestFile->text();

//    QJsonObject json1;
//    json1["LoadExpDataConfig"] = json;
//    SaveJsonToFile(json1, "LoadExpDataConfig.json");
}

// SIMULATION GUI
void MainWindow::writeSimSettingsToJson(QJsonObject &json, bool fVerbose)
{
    //qDebug() << "GUI->Sim Json";
  fVerbose = true;  //Now always!!!

  QJsonObject js;

  int versionNumber = ANTS2_VERSION;
  js["ANTS2build"] = versionNumber;

  SimGeneralConfigToJson(js);         //general sim settings
  if (ui->twSourcePhotonsParticles->currentIndex() == 0)
    { //point source sim
      js["Mode"] = "PointSim";
      SimPointSourcesConfigToJson(js, fVerbose);
      if (fVerbose) SimParticleSourcesConfigToJson(js);
    }
  else
    { //particle source sim
      js["Mode"] = "SourceSim";
      SimParticleSourcesConfigToJson(js);
      if (fVerbose) SimPointSourcesConfigToJson(js, fVerbose);
    }
  js["DoGuiUpdate"] = true;           //batcher have to set it to false!

  json["SimulationConfig"] = js;

  //QJsonObject js1;
  //js1["SimulationConfig"] = js;
  //SaveJsonToFile(js1, "SimConfig.json");
}

void MainWindow::onRequestSimulationGuiUpdate()
{
    //qDebug() << "SimJson->SimGUI";
    MainWindow::readSimSettingsFromJson(Config->JSON);
}

bool MainWindow::readSimSettingsFromJson(QJsonObject &json)
{
  //qDebug() << "Read sim from json and update sim gui";
  if (!json.contains("SimulationConfig"))
    {
      //qWarning() << "Json does not contain sim settings!";
      return false;
    }

  //cleanup  
  if (histScan) delete histScan;
  histScan = 0;
  if (histSecScint) delete histSecScint;
  histSecScint = 0;
  ui->pbScanDistrShow->setEnabled(false);
  ui->pbScanDistrDelete->setEnabled(false);
  populateTable = true;
  if (ParticleStack.size()>0)
    {
      for (int i=0; i<ParticleStack.size(); i++) delete ParticleStack[i];
      ParticleStack.resize(0);
    }

  DoNotUpdateGeometry = true;
  BulkUpdate = true;
  QJsonObject js = json["SimulationConfig"].toObject();

  //GENERAL SETTINGS
  QJsonObject gjs = js["GeneralSimConfig"].toObject();
  // wave
  QJsonObject wj = gjs["WaveConfig"].toObject();
  JsonToCheckbox(wj, "WaveResolved", ui->cbWaveResolved);
  JsonToLineEdit(wj, "WaveFrom", ui->ledWaveFrom);
  JsonToLineEdit(wj, "WaveTo", ui->ledWaveTo);
  JsonToLineEdit(wj, "WaveStep", ui->ledWaveStep);
  // time
  QJsonObject tj = gjs["TimeConfig"].toObject();
  JsonToCheckbox(tj, "TimeResolved", ui->cbTimeResolved);
  JsonToLineEdit(tj, "TimeFrom", ui->ledTimeFrom);
  JsonToLineEdit(tj, "TimeTo", ui->ledTimeTo);
  JsonToSpinBox (tj, "TimeBins", ui->sbTimeBins);
    //JsonToLineEdit(tj, "TimeWindow", ui->ledTimeOfOneMeasurement); //moved to detector config
  // Angular
  QJsonObject aj = gjs["AngleConfig"].toObject();
  JsonToCheckbox(aj, "AngResolved", ui->cbAngularSensitive);
  JsonToSpinBox (aj, "NumBins", ui->sbCosBins);
  // Area
  QJsonObject arj = gjs["AreaConfig"].toObject();
  JsonToCheckbox(arj, "AreaResolved", ui->cbAreaSensitive);
  // LRF based sim
  ui->cbLRFs->setChecked(false);
  if (gjs.contains("LrfBasedSim"))
  {
      QJsonObject lrfjson = gjs["LrfBasedSim"].toObject();
      JsonToCheckbox(lrfjson, "UseLRFs", ui->cbLRFs);
      JsonToSpinBox (lrfjson, "NumPhotsLRFunity", ui->sbPhotonsForUnitaryLRF);
      JsonToLineEdit(lrfjson, "NumPhotElLRFunity", ui->ledNumElPerUnitaryLRF);
  }
  //Tracking options
  QJsonObject trj = gjs["TrackingConfig"].toObject();
  JsonToLineEdit(trj, "MinStep", ui->ledMinStep);
  JsonToLineEdit(trj, "MaxStep", ui->ledMaxStep);
  JsonToLineEdit(trj, "dE", ui->ledDE);
  JsonToLineEdit(trj, "MinEnergy", ui->ledMinEnergy);
  JsonToLineEdit(trj, "Safety", ui->ledSafety);
  JsonToSpinBox(trj, "TrackColorAdd", ui->sbParticleTrackColorIndexAdd);
  //Accelerators
  QJsonObject acj = gjs["AcceleratorConfig"].toObject();
  JsonToSpinBox (acj, "MaxNumTransitions", ui->sbMaxNumbPhTransitions);
  JsonToCheckbox(acj, "CheckBeforeTrack", ui->cbRndCheckBeforeTrack);
  JsonToCheckbox(acj, "OnlyTracksOnPMs", ui->cbOnlyBuildTracksOnPMs);
  JsonToCheckbox(acj, "LogsStatistics", ui->cbDoLogsAndStatistics);
  //JsonToSpinBox(acj, "NumberThreads", ui->sbNumberThreads);
  //Sec scint
  QJsonObject scj = gjs["SecScintConfig"].toObject();
  JsonToComboBox(scj, "Type", ui->cobSecScintillationGenType);
  /*
if (scj.contains("CustomDistrib"))
  {
    QJsonArray ja = scj["CustomDistrib"].toArray();
    int size = ja.size();
    if (size>0)
      {
        double* xx = new double[size];
        double* yy = new double[size];
        for (int i=0; i<size; i++)
            {
              xx[i] = ja[i].toArray()[0].toDouble();
              yy[i] = ja[i].toArray()[1].toDouble();
            }
        histSecScint = new TH1D("SecScintTheta","SecScint: Theta", size-1, xx);
        for (int i = 1; i<size+1; i++)  histSecScint->SetBinContent(i, yy[i-1]);
        delete[] xx;
        delete[] yy;
      }
  }
*/
  JsonToCheckbox(gjs, "BuildPhotonTracks", ui->cbPointSourceBuildTracks); //general now

  //POINT SOURCES
  QJsonObject pojs = js["PointSourcesConfig"].toObject();
  //control
  QJsonObject pcj = pojs["ControlOptions"].toObject();
  int SimMode = pcj["Single_Scan_Flood"].toInt();
  if (SimMode>-1 && SimMode<ui->twSingleScan->count())
  {
      ui->twSingleScan->blockSignals(true);
      ui->twSingleScan->setCurrentIndex(SimMode);
      ui->twSingleScan->blockSignals(false);
  }  
  JsonToComboBox(pcj, "Primary_Secondary", ui->cobScintTypePointSource);
  JsonToCheckbox(pcj, "BuildTracks", ui->cbPointSourceBuildTracks);
  JsonToCheckbox(pcj, "MultipleRuns", ui->cbNumberOfRuns);
  JsonToSpinBox (pcj, "MultipleRunsNumber", ui->sbNumberOfRuns);
  ui->cbLimitNodesOutsideObject->setChecked(false);  //compatibility
  if (pcj.contains("GenerateOnlyInPrimary")) //old system
  {
      ui->cbLimitNodesOutsideObject->setChecked(true);
      ui->leLimitNodesObject->setText("PrScint");
  }
  if (pcj.contains("LimitNodesTo"))
  {
      if (pcj.contains("LimitNodes"))
          JsonToCheckbox(pcj, "LimitNodes", ui->cbLimitNodesOutsideObject); //new system
      else
          ui->cbLimitNodesOutsideObject->setChecked(true); //semi-old system
      ui->leLimitNodesObject->setText( pcj["LimitNodesTo"].toString() );
  }
  //potons per node;
  QJsonObject ppj = pojs["PhotPerNodeOptions"].toObject();
  JsonToComboBox(ppj, "PhotPerNodeMode", ui->cobScanNumPhotonsMode);
  JsonToSpinBox(ppj, "PhotPerNodeConstant", ui->sbScanNumPhotons);
  JsonToSpinBox(ppj, "PhotPerNodeUniMin", ui->sbScanNumMin);
  JsonToSpinBox(ppj, "PhotPerNodeUniMax", ui->sbScanNumMax);
  JsonToLineEdit(ppj, "PhotPerNodeGaussMean", ui->ledScanGaussMean);
  JsonToLineEdit(ppj, "PhotPerNodeGaussSigma", ui->ledScanGaussSigma);
  if (ppj.contains("PhotPerNodeCustom"))
    {
      QJsonArray ja = ppj["PhotPerNodeCustom"].toArray();
      int size = ja.size();
      if (size > 0)
        {
          double* xx = new double[size];
          int* yy    = new int[size];
          for (int i=0; i<size; i++)
            {
              xx[i] = ja[i].toArray()[0].toDouble();
              yy[i] = ja[i].toArray()[1].toInt();
            }
          histScan = new TH1I("histPhotDistr","Photon distribution", size-1, xx);          
          histScan->SetXTitle("Number of generated photons");
          histScan->SetYTitle("Relative probability");
          for (int i = 1; i<size+1; i++) histScan->SetBinContent(i, yy[i-1]);
          histScan->GetIntegral();
          delete[] xx;
          delete[] yy;
          ui->pbScanDistrShow->setEnabled(true);
          ui->pbScanDistrDelete->setEnabled(true);
        }
    }  
  //Wavelength/decay options
  QJsonObject wdj = pojs["WaveTimeOptions"].toObject();
  JsonToComboBox(wdj, "Direct_Material", ui->cobDirectlyOrFromMaterial);
  JsonToSpinBox (wdj, "WaveIndex", ui->sbWaveIndexPointSource);
  JsonToLineEdit(wdj, "DecayTime", ui->ledDecayTime);
  JsonToComboBox(wdj, "Material", ui->cobMatPointSource);
  //Photon direction options
  QJsonObject pdj = pojs["PhotonDirectionOptions"].toObject();
  JsonToLineEdit(pdj, "FixedX", ui->ledSingleDX);
  JsonToLineEdit(pdj, "FixedY", ui->ledSingleDY);
  JsonToLineEdit(pdj, "FixedZ", ui->ledSingleDZ);  
  JsonToLineEdit(pdj, "Cone", ui->ledConeAngle);
  ui->cobFixedDirOrCone->setCurrentIndex(0); //compatibility
  JsonToComboBox(pdj, "Fixed_or_Cone", ui->cobFixedDirOrCone);
  JsonToCheckbox(pdj, "Random", ui->cbRandomDir);
  //bad event config
  QJsonObject bej = pojs["BadEventOptions"].toObject();
  JsonToCheckbox(bej, "BadEvents", ui->cbScanFloodAddNoise);
  JsonToLineEdit(bej, "Probability", ui->leoScanFloodNoiseProbability);
  JsonToLineEdit(bej, "SigmaDouble", ui->ledScanFloodNoiseOffset);
  MainWindow::PointSource_InitTabWidget();
  if (bej.contains("NoiseArray"))
    {
      QJsonArray ar = bej["NoiseArray"].toArray();
      if (ar.size() == 6)
        {
          for (int ic=0; ic<6; ic++)
            {
              QJsonObject js = ar[ic].toObject();
              ScanFloodNoise[ic].Active = js["Active"].toBool();
              //ScanFloodNoise[ic].Description = js["Description"].toString(); //keep standart description!
              ScanFloodNoise[ic].Probability = js["Probability"].toDouble();
              ScanFloodNoise[ic].AverageValue = js["AverageValue"].toDouble();
              ScanFloodNoise[ic].Spread = js["Spread"].toDouble();
            }
        }      
      MainWindow::PointSource_UpdateTabWidget();
    }
  //Single position options
  QJsonObject spj = pojs["SinglePositionOptions"].toObject();
  JsonToLineEdit(spj, "SingleX", ui->ledSingleX);
  JsonToLineEdit(spj, "SingleY", ui->ledSingleY);
  JsonToLineEdit(spj, "SingleZ", ui->ledSingleZ);
  //Regular scan options
  QJsonObject rsj = pojs["RegularScanOptions"].toObject();
  JsonToLineEdit(rsj, "ScanX0", ui->ledOriginX);
  JsonToLineEdit(rsj, "ScanY0", ui->ledOriginY);
  JsonToLineEdit(rsj, "ScanZ0", ui->ledOriginZ);
  ui->cbSecondAxis->setChecked(false);
  ui->cbThirdAxis->setChecked(false);
  if (rsj.contains("AxesData"))
    {
      QJsonArray ar = rsj["AxesData"].toArray();
      if (ar.size()>0)
        {
          QJsonObject js = ar[0].toObject();
          JsonToLineEdit(js, "dX", ui->led0X);
          JsonToLineEdit(js, "dY", ui->led0Y);
          JsonToLineEdit(js, "dZ", ui->led0Z);
          JsonToSpinBox (js, "Nodes", ui->sb0nodes);
          JsonToComboBox(js, "Option", ui->cob0dir);
        }
      if (ar.size()>1)
        {
          QJsonObject js = ar[1].toObject();
          JsonToLineEdit(js, "dX", ui->led1X);
          JsonToLineEdit(js, "dY", ui->led1Y);
          JsonToLineEdit(js, "dZ", ui->led1Z);
          JsonToSpinBox (js, "Nodes", ui->sb1nodes);
          JsonToComboBox(js, "Option", ui->cob1dir);
          ui->cbSecondAxis->setChecked(true);
        }
      if (ar.size()>2)
        {
          QJsonObject js = ar[2].toObject();
          JsonToLineEdit(js, "dX", ui->led2X);
          JsonToLineEdit(js, "dY", ui->led2Y);
          JsonToLineEdit(js, "dZ", ui->led2Z);
          JsonToSpinBox (js, "Nodes", ui->sb2nodes);
          JsonToComboBox(js, "Option", ui->cob2dir);
          ui->cbThirdAxis->setChecked(true);
        }
    }  
  //Flood options
  QJsonObject fj = pojs["FloodOptions"].toObject();
  JsonToSpinBox (fj, "Nodes", ui->sbScanFloodNodes);
  JsonToComboBox(fj, "Shape", ui->cobScanFloodShape);
  JsonToLineEdit(fj, "Xfrom", ui->ledScanFloodXfrom);
  JsonToLineEdit(fj, "Xto", ui->ledScanFloodXto);
  JsonToLineEdit(fj, "Yfrom", ui->ledScanFloodYfrom);
  JsonToLineEdit(fj, "Yto", ui->ledScanFloodYto);
  JsonToLineEdit(fj, "CenterX", ui->ledScanFlood_CenterX);
  JsonToLineEdit(fj, "CenterY", ui->ledScanFlood_CenterY);
  JsonToLineEdit(fj, "DiameterOut", ui->ledScanFlood_Radius);
  JsonToLineEdit(fj, "DiameterIn", ui->ledScanFlood_Radius0);
  JsonToComboBox(fj, "Zoption", ui->cobScanFloodZtype);
  JsonToLineEdit(fj, "Zfixed", ui->ledScanFloodZ);
  JsonToLineEdit(fj, "Zfrom", ui->ledScanFloodZfrom);
  JsonToLineEdit(fj, "Zto", ui->ledScanFloodZto);
  //Custom nodes
  QJsonObject njson = pojs["CustomNodesOptions"].toObject();
  NodesScript.clear();
  parseJson(njson, "Script", NodesScript);
  QJsonArray arr;
  parseJson(njson, "Nodes", arr);
  clearCustomScanNodes();
  for (int i=0; i<arr.size(); i++)
    {
      QJsonArray el = arr[i].toArray();
      if (el.size()<3)
        {
          qWarning() << "Error in custom nodes array";
          break;
        }
      CustomScanNodes.append( new QVector3D(el[0].toDouble(), el[1].toDouble(), el[2].toDouble()));
    }
  ui->lScriptNodes->setText( QString::number(CustomScanNodes.size()) );

  //PARTICLE SOURCES
  QJsonObject psjs = js["ParticleSourcesConfig"].toObject();
  //full sources mode
  QJsonObject csjs = psjs["SourceControlOptions"].toObject();
  JsonToSpinBox (csjs, "EventsToDo", ui->sbGunEvents);
  JsonToCheckbox(csjs, "AllowMultipleParticles", ui->cbGunAllowMultipleEvents);
  JsonToLineEdit(csjs, "AverageParticlesPerEvent", ui->ledGunAverageNumPartperEvent);
  JsonToCheckbox(csjs, "DoS1", ui->cbGunDoS1);
  JsonToCheckbox(csjs, "DoS1", ui->cbDoS1tester);
  JsonToCheckbox(csjs, "DoS2", ui->cbGunDoS2);
  JsonToCheckbox(csjs, "DoS2", ui->cbDoS2tester);
  JsonToCheckbox(csjs, "ParticleTracks", ui->cbGunParticleTracks);
  JsonToCheckbox(csjs, "ParticleTracks", ui->cbBuildParticleTrackstester);
  JsonToCheckbox(csjs, "PhotonTracks", ui->cbGunPhotonTracks);
  JsonToCheckbox(csjs, "PhotonTracks", ui->cbBuilPhotonTrackstester);
  ui->cbIgnoreEventsWithNoHits->setChecked(false);//compatibility
  JsonToCheckbox(csjs, "IgnoreNoHitsEvents", ui->cbIgnoreEventsWithNoHits);
  ui->cbIgnoreEventsWithNoEnergyDepo->setChecked(true);//compatibility
  JsonToCheckbox(csjs, "IgnoreNoDepoEvents", ui->cbIgnoreEventsWithNoEnergyDepo);

  //particle sources
  int SelectedSource = ui->cobParticleSource->currentIndex();
  ParticleSources->clear();
  ParticleSources->readFromJson(psjs);
  on_pbUpdateSourcesIndication_clicked();
  if (SelectedSource>-1 && SelectedSource<ParticleSources->size())
  {
      ui->cobParticleSource->setCurrentIndex(SelectedSource);
      updateOneParticleSourcesIndication(ParticleSources->getSource(SelectedSource));
  }

  //Window CONTROL
  if (js.contains("Mode"))
    {
      ui->twSourcePhotonsParticles->blockSignals(true);
      QString Mode = js["Mode"].toString();
      if (Mode == "PointSim") ui->twSourcePhotonsParticles->setCurrentIndex(0);
      else if (Mode =="StackSim" || Mode =="SourceSim") ui->twSourcePhotonsParticles->setCurrentIndex(1);
      ui->twSourcePhotonsParticles->blockSignals(false);
    }

  DoNotUpdateGeometry = false;
  BulkUpdate = false;

  //updating global parameters
  MainWindow::on_cbWaveResolved_toggled(ui->cbWaveResolved->isChecked()); //update on toggle
  MainWindow::on_cbAngularSensitive_toggled(ui->cbAngularSensitive->isChecked());
  MainWindow::on_cbTimeResolved_toggled(ui->cbTimeResolved->isChecked());

  //update indication
  MainWindow::on_pbRefreshStack_clicked();

  return true;
}

void MainWindow::selectFirstActiveParticleSource()
{
    if (ParticleSources->getTotalActivity() > 0)
      {  //show the first source with non-zero activity
        int i=0;
        for (; i<ParticleSources->size(); i++)
          if (ParticleSources->getSource(i)->Activity > 0) break;
        ui->cobParticleSource->setCurrentIndex(i);        
      }
    else if (ParticleSources->size()>0) ui->cobParticleSource->setCurrentIndex(0);
    else ui->cobParticleSource->setCurrentIndex(-1);
    on_pbUpdateSourcesIndication_clicked();
}
