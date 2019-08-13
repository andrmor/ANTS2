//ANTS2 modules and windows
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aglobalsettings.h"
#include "ajsontools.h"
#include "amessage.h"
#include "asimulationmanager.h"
#include "alogsandstatisticsoptions.h"

//Qt
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QVector3D>

void MainWindow::SimGeneralConfigToJson(QJsonObject &jsonMaster)
{
  QJsonObject json;

  //Wavelength options
  QJsonObject wjson;
      wjson["WaveResolved"] = ui->cbWaveResolved->isChecked();
      wjson["WaveFrom"] = ui->ledWaveFrom->text().toDouble();
      wjson["WaveTo"] = ui->ledWaveTo->text().toDouble();
      wjson["WaveStep"] = ui->ledWaveStep->text().toDouble();
      wjson["WaveNodes"] = WaveNodes;
  json["WaveConfig"] = wjson;

  //Time options
  QJsonObject tjson;
      tjson["TimeResolved"] = ui->cbTimeResolved->isChecked();
      tjson["TimeFrom"] = ui->ledTimeFrom->text().toDouble();
      tjson["TimeTo"] = ui->ledTimeTo->text().toDouble();
      tjson["TimeBins"] = ui->sbTimeBins->value();
  json["TimeConfig"] = tjson;

  //Angular options
  QJsonObject ajson;
      ajson["AngResolved"] = ui->cbAngularSensitive->isChecked();
      ajson["NumBins"] = ui->sbCosBins->value();
  json["AngleConfig"] = ajson;

  //Area options
  QJsonObject arjson;
  arjson["AreaResolved"] = ui->cbAreaSensitive->isChecked();
  json["AreaConfig"] = arjson;

  //LRF-based sim
  QJsonObject lrfjson;
  lrfjson["UseLRFs"] = ui->cbLRFs->isChecked();
  lrfjson["NumPhotsLRFunity"] = ui->sbPhotonsForUnitaryLRF->value();
  lrfjson["NumPhotElLRFunity"] = ui->ledNumElPerUnitaryLRF->text().toDouble();
  json["LrfBasedSim"] = lrfjson;

  //Tracking options
  QJsonObject trjson;
  trjson["MinStep"] = ui->ledMinStep->text().toDouble();
  trjson["MaxStep"] = ui->ledMaxStep->text().toDouble();
  trjson["dE"] = ui->ledDE->text().toDouble();
  trjson["MinEnergy"] = ui->ledMinEnergy->text().toDouble();
  trjson["MinEnergyNeutrons"] = ui->ledMinEnergyNeutrons->text().toDouble();
  trjson["Safety"] = ui->ledSafety->text().toDouble();
  json["TrackingConfig"] = trjson;

  //Accelerators options
  QJsonObject acjson;
  acjson["MaxNumTransitions"] = ui->sbMaxNumbPhTransitions->value();
  acjson["CheckBeforeTrack"] = ui->cbRndCheckBeforeTrack->isChecked();
  json["AcceleratorConfig"] = acjson;

  //DetStat binning
  json["DetStatNumBins"] = GlobSet.BinsX;

  //Sec scint options
  QJsonObject scjson;
      int Type = ui->cobSecScintillationGenType->currentIndex(); //0-4Pi, 1-2PiUp, 2-2Pidown, 3-custom
      scjson["Type"] = Type;      
//      if (Type == 3)
//        {
//          //saving hist data to JsonArray
//          if (histSecScint)
//            {
//              QJsonArray ar;
//              writeTH1DtoJsonArr(histSecScint, ar);
//              scjson["CustomDistrib"] = ar;
//            }
//        }
  json["SecScintConfig"] = scjson;

  QJsonObject tbojs;
    SimulationManager->TrackBuildOptions.writeToJson(tbojs);
  json["TrackBuildingOptions"] = tbojs;

  QJsonObject lsjs;
    SimulationManager->LogsStatOptions.writeToJson(lsjs);
  json["LogStatOptions"] = lsjs;

  QJsonObject g4js;
    G4SimSet.bTrackParticles = ui->cbGeant4ParticleTracking->isChecked();
    G4SimSet.writeToJson(g4js);
  json["Geant4SimulationSettings"] = g4js;

  //adding to master json
  jsonMaster["GeneralSimConfig"] = json;
}

struct SAxis
{
  double dX, dY, dZ;
  int nodes;
  int option;

  SAxis(){}
  SAxis(double DX, double DY, double DZ, int Nodes, int Option){dX=DX; dY=DY; dZ=DZ; nodes=Nodes; option=Option;}
};

void MainWindow::SimPointSourcesConfigToJson(QJsonObject &jsonMaster)
{
  QJsonObject json;

  //main control options
  QJsonObject cjson;
  int SimMode = ui->twSingleScan->currentIndex();
    cjson["Single_Scan_Flood"] = SimMode;    
    cjson["Primary_Secondary"] = ui->cobScintTypePointSource->currentIndex();
    cjson["MultipleRuns"] = ui->cbNumberOfRuns->isChecked();
    cjson["MultipleRunsNumber"] = ui->sbNumberOfRuns->value();
    cjson["LimitNodes"] = ui->cbLimitNodesOutsideObject->isChecked();
    cjson["LimitNodesTo"] = ui->leLimitNodesObject->text();
  json["ControlOptions"] = cjson;

  //potons per node;
  QJsonObject ppnjson;
  int ModePG = ui->cobScanNumPhotonsMode->currentIndex();
  ppnjson["PhotPerNodeMode"] = ModePG;
  ppnjson["PhotPerNodeConstant"] = ui->sbScanNumPhotons->value();
  ppnjson["PhotPerNodeUniMin"] = ui->sbScanNumMin->value();
  ppnjson["PhotPerNodeUniMax"] = ui->sbScanNumMax->value();
  ppnjson["PhotPerNodeGaussMean"] = ui->ledScanGaussMean->text().toDouble();
  ppnjson["PhotPerNodeGaussSigma"] = ui->ledScanGaussSigma->text().toDouble();
  if (histScan)
  {
      QJsonArray ja;
      writeTH1ItoJsonArr(histScan, ja);
      ppnjson["PhotPerNodeCustom"] = ja;
  }
  else ppnjson["PhotPerNodeCustom"] = QJsonArray();
  json["PhotPerNodeOptions"] = ppnjson;

  //Wavelength/decay options
  QJsonObject wdjson;
      wdjson["UseFixedWavelength"] = ui->cbFixWavelengthPointSource->isChecked();
      wdjson["WaveIndex"] = ui->sbFixedWaveIndexPointSource->value();
  json["WaveTimeOptions"] = wdjson;

  //Photon direction options
  QJsonObject pdjson;
  pdjson["Random"] = ui->cbRandomDir->isChecked();
  pdjson["FixedX"] = ui->ledSingleDX->text().toDouble();
  pdjson["FixedY"] = ui->ledSingleDY->text().toDouble();
  pdjson["FixedZ"] = ui->ledSingleDZ->text().toDouble();
  pdjson["Fixed_or_Cone"] = ui->cobFixedDirOrCone->currentIndex();
  pdjson["Cone"] = ui->ledConeAngle->text().toDouble();
  json["PhotonDirectionOptions"] = pdjson;

  QJsonObject spjson;
      spjson["SingleX"] = ui->ledSingleX->text().toDouble();
      spjson["SingleY"] = ui->ledSingleY->text().toDouble();
      spjson["SingleZ"] = ui->ledSingleZ->text().toDouble();
  json["SinglePositionOptions"] = spjson;

  QJsonObject rsjson;
      rsjson["ScanX0"] = ui->ledOriginX->text().toDouble();
      rsjson["ScanY0"] = ui->ledOriginY->text().toDouble();
      rsjson["ScanZ0"] = ui->ledOriginZ->text().toDouble();
      QVector<SAxis> sa;
      sa.append(SAxis(ui->led0X->text().toDouble(),
                      ui->led0Y->text().toDouble(),
                      ui->led0Z->text().toDouble(),
                      ui->sb0nodes->value(), ui->cob0dir->currentIndex()));
      if (ui->cbSecondAxis->isChecked())
        sa.append(SAxis(ui->led1X->text().toDouble(),
                         ui->led1Y->text().toDouble(),
                         ui->led1Z->text().toDouble(),
                         ui->sb1nodes->value(), ui->cob1dir->currentIndex()));
      if (ui->cbThirdAxis->isChecked())
         sa.append(SAxis(ui->led2X->text().toDouble(),
                         ui->led2Y->text().toDouble(),
                         ui->led2Z->text().toDouble(),
                         ui->sb2nodes->value(), ui->cob2dir->currentIndex()));
      QJsonArray rsdataArr;
      for (int ia=0; ia<sa.size(); ia++)
      {
          QJsonObject adjson;
          adjson["dX"] = sa[ia].dX;
          adjson["dY"] = sa[ia].dY;
          adjson["dZ"] = sa[ia].dZ;
          adjson["Nodes"] = sa[ia].nodes;
          adjson["Option"] = sa[ia].option;
          rsdataArr.append(adjson);
      }
      rsjson["AxesData"] = rsdataArr;
  json["RegularScanOptions"] = rsjson;

  //Flood options
  QJsonObject fjson;
      fjson["Nodes"] = ui->sbScanFloodNodes->value();
      fjson["Shape"] = ui->cobScanFloodShape->currentIndex();
      fjson["Xfrom"] = ui->ledScanFloodXfrom->text().toDouble();
      fjson["Xto"] = ui->ledScanFloodXto->text().toDouble();
      fjson["Yfrom"] = ui->ledScanFloodYfrom->text().toDouble();
      fjson["Yto"] = ui->ledScanFloodYto->text().toDouble();
      fjson["CenterX"] = ui->ledScanFlood_CenterX->text().toDouble();
      fjson["CenterY"] = ui->ledScanFlood_CenterY->text().toDouble();
      fjson["DiameterOut"] = ui->ledScanFlood_Radius->text().toDouble();
      fjson["DiameterIn"] = ui->ledScanFlood_Radius0->text().toDouble();
      fjson["Zoption"] = ui->cobScanFloodZtype->currentIndex();
      fjson["Zfixed"] = ui->ledScanFloodZ->text().toDouble();
      fjson["Zfrom"] = ui->ledScanFloodZfrom->text().toDouble();
      fjson["Zto"] = ui->ledScanFloodZto->text().toDouble();
  json["FloodOptions"] = fjson;

  // custom nodes
  QJsonObject ndjson;
      ndjson["FileWithNodes"] = ui->leNodesFromFile->text();
  json["CustomNodesOptions"] = ndjson;

  //adding to master json
  jsonMaster["PointSourcesConfig"] = json;
}
