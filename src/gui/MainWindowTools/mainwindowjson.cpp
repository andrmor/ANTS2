#include "mainwindow.h"
#include "aparticlesourcerecord.h"
#include "aglobalsettings.h"
#include "ui_mainwindow.h"
#include "detectorclass.h"
#include "asandwich.h"
#include "materialinspectorwindow.h"
#include "amaterialparticlecolection.h"
#include "detectoraddonswindow.h"
#include "outputwindow.h"
#include "geometrywindowclass.h"
#include "aconfiguration.h"
#include "ajsontools.h"
#include "asourceparticlegenerator.h"
#include "afileparticlegenerator.h"
#include "ascriptparticlegenerator.h"
#include "apmhub.h"
#include "eventsdataclass.h"
#include "tmpobjhubclass.h"
#include "reconstructionwindow.h"
#include "asimulationmanager.h"
#include "aparticlerecord.h"

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

void MainWindow::onRequestDetectorGuiUpdate()
{
    //qDebug() << "DetJson->DetGUI";
  Detector->GeoManager->SetNsegments(GlobSet.NumSegments); //lost because GeoManager is new

  DoNotUpdateGeometry = true;
  //Particles
  ListActiveParticles();
  //Materials
  on_pbRefreshMaterials_clicked();
  MIwindow->SetLogLog(Detector->MpCollection->fLogLogInterpolation);
  //Optical overrides
  on_pbRefreshOverrides_clicked();
  //PM types
  if (ui->sbPMtype->value() > PMs->countPMtypes()-1) ui->sbPMtype->setValue(0);
  updateCOBsWithPMtypeNames();
  on_pbRefreshPMproperties_clicked();
  //PM arrays
  ui->cbUPM->setChecked(Detector->PMarrays[0].fActive);
  ui->cbLPM->setChecked(Detector->PMarrays[1].fActive);
  on_pbShowPMsArrayRegularData_clicked(); //update regular array fields
  //Detector addon window updates
  DAwindow->UpdateGUI();
  if (Detector->PMdummies.size()>0) DAwindow->UpdateDummyPMindication();
  //Output window
  Owindow->RefreshData();
//  Owindow->ResetViewport();  // *** !!! extend to other windows too!
  //world
  const double WorldSizeXY = Detector->Sandwich->getWorldSizeXY();
  const double WorldSizeZ  = Detector->Sandwich->getWorldSizeZ();
  ui->ledTopSizeXY->setText( QString::number(2.0 * WorldSizeXY, 'g', 4) );
  ui->ledTopSizeZ-> setText( QString::number(2.0 * WorldSizeZ,  'g', 4) );
  ui->cbFixedTopSize->setChecked( Detector->Sandwich->isWorldSizeFixed() );
  //misc
  ShowPMcount();
  //CheckPresenseOfSecScintillator(); //checks if SecScint present, and update selectors of primary/secondary scintillation  //***!!! potentially slow on large geometries!!!
  //Sandwich - now automatic
  //PM explore
  on_pbIndPMshowInfo_clicked();
  //Electronics
  on_pbElUpdateIndication_clicked();
  //Gains
  on_pbGainsUpdateGUI_clicked();
  //GDML?
  onGDMLstatusChage(!Detector->isGDMLempty());

  //Detector->checkSecScintPresent();
  //ui->fSecondaryLightGenType->setEnabled(Detector->fSecScintPresent);

  //readExtraGuiFromJson(Config->JSON); //new!
  //qDebug() << "Before:\n"<<Config->JSON["DetectorConfig"].toObject()["LoadExpDataConfig"].toObject();
  UpdatePreprocessingSettingsIndication();
  //qDebug() << "AFTER:\n"<<Config->JSON["DetectorConfig"].toObject()["LoadExpDataConfig"].toObject();

  DoNotUpdateGeometry = false;

  if (GeometryWindow->isVisible())
  {
      GeometryWindow->ShowGeometry(false);
      GeometryWindow->DrawTracks();
  }
}

void MainWindow::onNewConfigLoaded()
{
    //qDebug() << "-->New detector config was loaded";
    initOverridesAfterLoad();

    readExtraGuiFromJson(Config->JSON);

    Owindow->ResetViewport();
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

        QJsonObject jeom;
            GeometryWindow->writeToJson(jeom);
        jmain["GeometryWindow"] = jeom;

        Rwindow->writeMiscGUIsettingsToJson(jmain);  //Misc setting (PlotXY, blur)

        QJsonObject jOW;
        Owindow->SaveGuiToJson(jOW);
        jmain["OutputWindow"] = jOW;

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

    QJsonObject js;
    if ( parseJson(jmain, "GeometryWindow", js) ) GeometryWindow->readFromJson(js);

    if (jmain.contains("ReconstructionWindow"))
        Rwindow->readMiscGUIsettingsFromJson(jmain);

    QJsonObject jOW;
    parseJson(jmain, "OutputWindow", jOW);
    if (!jOW.isEmpty()) Owindow->LoadGuiFromJson(jOW);
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
void MainWindow::writeSimSettingsToJson(QJsonObject &json)
{
    //qDebug() << "GUI->Sim Json";
    QJsonObject js;
    SimGeneralConfigToJson(js);         //general sim settings
    if (ui->twSourcePhotonsParticles->currentIndex() == 0)
        js["Mode"] = "PointSim"; //point source sim
    else
        js["Mode"] = "SourceSim"; //particle source sim
    SimPointSourcesConfigToJson(js);
    SimParticleSourcesConfigToJson(js);

    js["DoGuiUpdate"] = true;

    json["SimulationConfig"] = js;
}

void MainWindow::onRequestSimulationGuiUpdate()
{
    //      qDebug() << "SimJson->SimGUI";
    MainWindow::readSimSettingsFromJson(Config->JSON);
}

bool MainWindow::readSimSettingsFromJson(QJsonObject &json)
{
  //qDebug() << "Read sim from json and update sim gui";
  if (!json.contains("SimulationConfig"))
  {
      qWarning() << "Json does not contain sim settings!";
      return false;
  }

  //cleanup  
  delete histScan; histScan = nullptr;
  ui->pbScanDistrShow->setEnabled(false);
  ui->pbScanDistrDelete->setEnabled(false);
  populateTable = true;

  DoNotUpdateGeometry = true;
  BulkUpdate = true;
  QJsonObject js = json["SimulationConfig"].toObject();

  //GENERAL SETTINGS
  QJsonObject gjs = js["GeneralSimConfig"].toObject();
  // wave
  QJsonObject wj = gjs["WaveConfig"].toObject();
  JsonToCheckbox(wj, "WaveResolved", ui->cbWaveResolved);
  JsonToLineEditDouble(wj, "WaveFrom", ui->ledWaveFrom);
  JsonToLineEditDouble(wj, "WaveTo", ui->ledWaveTo);
  JsonToLineEditDouble(wj, "WaveStep", ui->ledWaveStep);
  // time
  QJsonObject tj = gjs["TimeConfig"].toObject();
  JsonToCheckbox(tj, "TimeResolved", ui->cbTimeResolved);
  JsonToLineEditDouble(tj, "TimeFrom", ui->ledTimeFrom);
  JsonToLineEditDouble(tj, "TimeTo", ui->ledTimeTo);
  JsonToSpinBox (tj, "TimeBins", ui->sbTimeBins);
    //JsonToLineEditDouble(tj, "TimeWindow", ui->ledTimeOfOneMeasurement); //moved to detector config
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
      JsonToLineEditDouble(lrfjson, "NumPhotElLRFunity", ui->ledNumElPerUnitaryLRF);
  }
  //Tracking options
  QJsonObject trj = gjs["TrackingConfig"].toObject();
  JsonToLineEditDouble(trj, "MinStep", ui->ledMinStep);
  JsonToLineEditDouble(trj, "MaxStep", ui->ledMaxStep);
  JsonToLineEditDouble(trj, "dE", ui->ledDE);
  JsonToLineEditDouble(trj, "MinEnergy", ui->ledMinEnergy);
  JsonToLineEditDouble(trj, "MinEnergyNeutrons", ui->ledMinEnergyNeutrons);
  JsonToLineEditDouble(trj, "Safety", ui->ledSafety);
  //Accelerators
  QJsonObject acj = gjs["AcceleratorConfig"].toObject();
  JsonToSpinBox (acj, "MaxNumTransitions", ui->sbMaxNumbPhTransitions);
  JsonToCheckbox(acj, "CheckBeforeTrack", ui->cbRndCheckBeforeTrack);
  //Sec scint
  //QJsonObject scj = gjs["SecScintConfig"].toObject();
  //JsonToComboBox(scj, "Type", ui->cobSecScintillationGenType);

  QJsonObject tbojs;
    parseJson(gjs, "TrackBuildingOptions", tbojs);
  SimulationManager->TrackBuildOptions.readFromJson(tbojs);

  QJsonObject lsjs;
    parseJson(gjs, "LogStatOptions", lsjs);
  SimulationManager->LogsStatOptions.readFromJson(lsjs);

  G4SimSet = AG4SimulationSettings();
  QJsonObject g4js;
  if (parseJson(gjs, "Geant4SimulationSettings", g4js))
      G4SimSet.readFromJson(g4js);
  ui->cbGeant4ParticleTracking->setChecked(G4SimSet.bTrackParticles);

  ExitParticleSettings.SaveParticles = false;
  {
      QJsonObject js;
        bool bOK = parseJson(gjs, "ExitParticleSettings", js);
      if (bOK) ExitParticleSettings.readFromJson(js);
  }
  ui->labParticlesToFile->setVisible(ExitParticleSettings.SaveParticles);

  //POINT SOURCES
  QJsonObject pojs = js["PointSourcesConfig"].toObject();
  //control
  QJsonObject pcj = pojs["ControlOptions"].toObject();
  int SimMode = pcj["Single_Scan_Flood"].toInt();
  if (SimMode > -1 && SimMode < ui->cobNodeGenerationMode->count())
  {
      //ui->cobNodeGenerationMode->blockSignals(true);
      ui->cobNodeGenerationMode->setCurrentIndex(SimMode);
      //ui->cobNodeGenerationMode->blockSignals(false);
  }  
  JsonToComboBox(pcj, "Primary_Secondary", ui->cobScintTypePointSource);
  //JsonToCheckbox(pcj, "BuildTracks", ui->cbPointSourceBuildTracks);
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
  JsonToLineEditDouble(ppj, "PhotPerNodeGaussMean", ui->ledScanGaussMean);
  JsonToLineEditDouble(ppj, "PhotPerNodeGaussSigma", ui->ledScanGaussSigma);
  JsonToLineEditDouble(ppj, "PhotPerNodePoissonMean", ui->ledScanPoissonMean);
  if (ppj.contains("PhotPerNodeCustom"))
  {
      QJsonArray ja = ppj["PhotPerNodeCustom"].toArray();
      int size = ja.size();
      if (size > 0)
      {
          double * xx = new double[size];
          double * yy = new double[size];
          for (int i=0; i<size; i++)
          {
              xx[i] = ja[i].toArray()[0].toDouble();
              yy[i] = ja[i].toArray()[1].toDouble();
          }
          histScan = new TH1D("","CustomNumPhotDist", size-1, xx);
          histScan->SetXTitle("Number of generated photons");
          histScan->SetYTitle("Relative probability, a.u.");
          for (int i = 1; i < size + 1; i++) histScan->SetBinContent(i, yy[i-1]);
          histScan->GetIntegral();
          delete[] xx;
          delete[] yy;
          ui->pbScanDistrShow->setEnabled(true);
          ui->pbScanDistrDelete->setEnabled(true);
      }
  }
  //Wavelength/decay options
  QJsonObject wdj = pojs["WaveTimeOptions"].toObject();
  ui->cbFixWavelengthPointSource->setChecked(false);  //compatibility
  JsonToCheckbox(wdj, "UseFixedWavelength", ui->cbFixWavelengthPointSource);
  JsonToSpinBox(wdj, "WaveIndex", ui->sbFixedWaveIndexPointSource);
  //Photon direction options
  QJsonObject pdj = pojs["PhotonDirectionOptions"].toObject();
  JsonToLineEditDouble(pdj, "FixedX", ui->ledSingleDX);
  JsonToLineEditDouble(pdj, "FixedY", ui->ledSingleDY);
  JsonToLineEditDouble(pdj, "FixedZ", ui->ledSingleDZ);
  JsonToLineEditDouble(pdj, "Cone", ui->ledConeAngle);
  ui->cobFixedDirOrCone->setCurrentIndex(0); //compatibility
  JsonToComboBox(pdj, "Fixed_or_Cone", ui->cobFixedDirOrCone);
  JsonToCheckbox(pdj, "Random", ui->cbRandomDir);

  //testing new system, to be updated later!
  QJsonObject sdsJson;
  if (parseJson(pojs, "SpatialDistOptions", sdsJson))
    SimulationManager->Settings.photSimSet.SpatialDistSettings.readFromJson(sdsJson);
  else SimulationManager->Settings.photSimSet.SpatialDistSettings.clearSettings();
  updateCNDgui();

  //Single position options
  QJsonObject spj = pojs["SinglePositionOptions"].toObject();
  JsonToLineEditDouble(spj, "SingleX", ui->ledSingleX);
  JsonToLineEditDouble(spj, "SingleY", ui->ledSingleY);
  JsonToLineEditDouble(spj, "SingleZ", ui->ledSingleZ);
  //Regular scan options
  QJsonObject rsj = pojs["RegularScanOptions"].toObject();
  JsonToLineEditDouble(rsj, "ScanX0", ui->ledOriginX);
  JsonToLineEditDouble(rsj, "ScanY0", ui->ledOriginY);
  JsonToLineEditDouble(rsj, "ScanZ0", ui->ledOriginZ);
  ui->cbSecondAxis->setChecked(false);
  ui->cbThirdAxis->setChecked(false);
  if (rsj.contains("AxesData"))
    {
      QJsonArray ar = rsj["AxesData"].toArray();
      if (ar.size()>0)
        {
          QJsonObject js = ar[0].toObject();
          JsonToLineEditDouble(js, "dX", ui->led0X);
          JsonToLineEditDouble(js, "dY", ui->led0Y);
          JsonToLineEditDouble(js, "dZ", ui->led0Z);
          JsonToSpinBox (js, "Nodes", ui->sb0nodes);
          JsonToComboBox(js, "Option", ui->cob0dir);
        }
      if (ar.size()>1)
        {
          QJsonObject js = ar[1].toObject();
          JsonToLineEditDouble(js, "dX", ui->led1X);
          JsonToLineEditDouble(js, "dY", ui->led1Y);
          JsonToLineEditDouble(js, "dZ", ui->led1Z);
          JsonToSpinBox (js, "Nodes", ui->sb1nodes);
          JsonToComboBox(js, "Option", ui->cob1dir);
          ui->cbSecondAxis->setChecked(true);
        }
      if (ar.size()>2)
        {
          QJsonObject js = ar[2].toObject();
          JsonToLineEditDouble(js, "dX", ui->led2X);
          JsonToLineEditDouble(js, "dY", ui->led2Y);
          JsonToLineEditDouble(js, "dZ", ui->led2Z);
          JsonToSpinBox (js, "Nodes", ui->sb2nodes);
          JsonToComboBox(js, "Option", ui->cob2dir);
          ui->cbThirdAxis->setChecked(true);
        }
    }  
  //Flood options
  QJsonObject fj = pojs["FloodOptions"].toObject();
  JsonToSpinBox (fj, "Nodes", ui->sbScanFloodNodes);
  JsonToComboBox(fj, "Shape", ui->cobScanFloodShape);
  JsonToLineEditDouble(fj, "Xfrom", ui->ledScanFloodXfrom);
  JsonToLineEditDouble(fj, "Xto", ui->ledScanFloodXto);
  JsonToLineEditDouble(fj, "Yfrom", ui->ledScanFloodYfrom);
  JsonToLineEditDouble(fj, "Yto", ui->ledScanFloodYto);
  JsonToLineEditDouble(fj, "CenterX", ui->ledScanFlood_CenterX);
  JsonToLineEditDouble(fj, "CenterY", ui->ledScanFlood_CenterY);
  JsonToLineEditDouble(fj, "DiameterOut", ui->ledScanFlood_Radius);
  JsonToLineEditDouble(fj, "DiameterIn", ui->ledScanFlood_Radius0);
  JsonToComboBox(fj, "Zoption", ui->cobScanFloodZtype);
  JsonToLineEditDouble(fj, "Zfixed", ui->ledScanFloodZ);
  JsonToLineEditDouble(fj, "Zfrom", ui->ledScanFloodZfrom);
  JsonToLineEditDouble(fj, "Zto", ui->ledScanFloodZto);
  //Custom nodes
  QJsonObject njson = pojs["CustomNodesOptions"].toObject();
  JsonToLineEditText(njson, "FileWithNodes", ui->leNodesFromFile);

  ui->labPhTracksOn->setVisible(SimulationManager->TrackBuildOptions.bBuildPhotonTracks);

    //PARTICLE SOURCES
    QJsonObject psjs = js["ParticleSourcesConfig"].toObject();
        QJsonObject csjs = psjs["SourceControlOptions"].toObject();
            //Particle generation mode
            QString PartGenMode = "Sources"; //compatibility
            parseJson(csjs, "ParticleGenerationMode", PartGenMode);
            int PGMindex = 0;
            if      (PartGenMode == "Sources") PGMindex = 0;
            else if (PartGenMode == "File")    PGMindex = 1;
            else if (PartGenMode == "Script")  PGMindex = 2;
            else qWarning() << "Load sim settings: Unknown particle generation mode!";
            ui->cobParticleGenerationMode->setCurrentIndex(PGMindex);
            JsonToSpinBox (csjs, "EventsToDo", ui->sbGunEvents);
            JsonToCheckbox(csjs, "AllowMultipleParticles", ui->cbGunAllowMultipleEvents);
            JsonToLineEditDouble(csjs, "AverageParticlesPerEvent", ui->ledGunAverageNumPartperEvent);
            ui->cobPartPerEvent->setCurrentIndex(0);
            JsonToComboBox(csjs, "TypeParticlesPerEvent", ui->cobPartPerEvent);
            JsonToCheckbox(csjs, "DoS1", ui->cbGunDoS1);
            JsonToCheckbox(csjs, "DoS2", ui->cbGunDoS2);
            ui->cbIgnoreEventsWithNoHits->setChecked(false);//compatibility
            JsonToCheckbox(csjs, "IgnoreNoHitsEvents", ui->cbIgnoreEventsWithNoHits);
            ui->cbIgnoreEventsWithNoEnergyDepo->setChecked(false);
            JsonToCheckbox(csjs, "IgnoreNoDepoEvents", ui->cbIgnoreEventsWithNoEnergyDepo);
            JsonToLineEditDouble(csjs, "ClusterMergeRadius", ui->ledClusterRadius);
            JsonToCheckbox(csjs, "ClusterMerge", ui->cbMergeClusters);
            JsonToLineEditDouble(csjs, "ClusterMergeTime", ui->ledClusterTimeDif);

        //particle sources
        SimulationManager->Settings.partSimSet.SourceGenSettings.readFromJson(psjs);
        //SimulationManager->ParticleSources->readFromJson(psjs);
        on_pbUpdateSourcesIndication_clicked();

        //generation from file
        QJsonObject fjs;
        parseJson(psjs, "GenerationFromFile", fjs);
            if (!fjs.isEmpty()) SimulationManager->Settings.partSimSet.FileGenSettings.readFromJson(fjs);
        updateFileParticleGeneratorGui();

        QJsonObject sjs;
        parseJson(psjs, "GenerationFromScript", sjs);
            if (!sjs.isEmpty()) SimulationManager->Settings.partSimSet.ScriptGenSettings.readFromJson(sjs);
        updateScriptParticleGeneratorGui();

        ui->labPhTracksOn_1->setVisible(SimulationManager->TrackBuildOptions.bBuildPhotonTracks);
        ui->labPartTracksOn->setVisible(SimulationManager->TrackBuildOptions.bBuildParticleTracks);
        ui->labPartLogOn->setVisible(SimulationManager->LogsStatOptions.bParticleTransportLog);


  //mode control
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
  WaveFrom = ui->ledWaveFrom->text().toDouble(); //***!!!
  WaveStep = ui->ledWaveStep->text().toDouble();
  CorrectWaveTo(); //WaveTo and WaveNode are set here
  on_pbIndPMshowInfo_clicked(); //to refresh the binned button

  //update indication
  on_pbYellow_clicked(); //yellow marker for activated advanced options in point source sim
  updateG4ProgressBarVisibility();
  UpdateTestWavelengthProperties();

  bool bWaveRes = ui->cbWaveResolved->isChecked();
  ui->fWaveTests->setEnabled(bWaveRes);
  ui->fWaveOptions->setEnabled(bWaveRes);
  ui->cbFixWavelengthPointSource->setEnabled(bWaveRes);

  return true;
}

void MainWindow::selectFirstActiveParticleSource()
{
    if (SimulationManager->Settings.partSimSet.SourceGenSettings.getTotalActivity() > 0)
    {
        //show the first source with non-zero activity
        int i = 0;
        for (; i < SimulationManager->Settings.partSimSet.SourceGenSettings.getNumSources(); i++)
            if (SimulationManager->Settings.partSimSet.SourceGenSettings.getSourceRecord(i)->Activity > 0) break;

        if (i < ui->lwDefinedParticleSources->count())
            ui->lwDefinedParticleSources->setCurrentRow(i);
    }
    on_pbUpdateSourcesIndication_clicked();
}
