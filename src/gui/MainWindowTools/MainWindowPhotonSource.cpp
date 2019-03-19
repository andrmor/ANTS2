//ANTS2 modules and windows
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aglobalsettings.h"
#include "ajsontools.h"
#include "amessage.h"
#include "simulationmanager.h"

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
  //acjson["OnlyTracksOnPMs"] = ui->cbOnlyBuildTracksOnPMs->isChecked();
  acjson["LogsStatistics"] = ui->cbDoLogsAndStatistics->isChecked();
  //acjson["NumberThreads"] = ui->sbNumberThreads->value();
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
  bool fVerbose = true;
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
  //switch (ModePG)
  //  {
  //  case 0:  //constant
      ppnjson["PhotPerNodeConstant"] = ui->sbScanNumPhotons->value();
  //    break;
  //  case 1:  //uniform
      ppnjson["PhotPerNodeUniMin"] = ui->sbScanNumMin->value();
      ppnjson["PhotPerNodeUniMax"] = ui->sbScanNumMax->value();
  //    break;
  //  case 2:  //Gauss
      ppnjson["PhotPerNodeGaussMean"] = ui->ledScanGaussMean->text().toDouble();
      ppnjson["PhotPerNodeGaussSigma"] = ui->ledScanGaussSigma->text().toDouble();
  //    break;
  //  case 3:  //Custom distribution
      if (histScan)
        {
          QJsonArray ja;
          writeTH1ItoJsonArr(histScan, ja);
          ppnjson["PhotPerNodeCustom"] = ja;
        }
      else ppnjson["PhotPerNodeCustom"] = QJsonArray();
   //   break;
   // default:
   //   qWarning("Unknown PhotonPerNode generation scheme!");
   // }
  json["PhotPerNodeOptions"] = ppnjson;

  //Wavelength/decay options
  QJsonObject wdjson;
      wdjson["UseFixedWavelength"] = ui->cbFixWavelengthPointSource->isChecked();
      wdjson["WaveIndex"] = ui->sbFixedWaveIndexPointSource->value();
  json["WaveTimeOptions"] = wdjson;

  //Photon direction options
  QJsonObject pdjson;
  bool fRandom = ui->cbRandomDir->isChecked();
  pdjson["Random"] = fRandom;
  //if (!fRandom)
  //  {
      pdjson["FixedX"] = ui->ledSingleDX->text().toDouble();
      pdjson["FixedY"] = ui->ledSingleDY->text().toDouble();
      pdjson["FixedZ"] = ui->ledSingleDZ->text().toDouble();
  //  }
  pdjson["Fixed_or_Cone"] = ui->cobFixedDirOrCone->currentIndex();
  pdjson["Cone"] = ui->ledConeAngle->text().toDouble();
  json["PhotonDirectionOptions"] = pdjson;

  //noise event config
  QJsonObject njson;
  bool fBadEvents = ui->cbScanFloodAddNoise->isChecked();
  njson["BadEvents"] = fBadEvents;
  //if (fBadEvents)
  //  {
      njson["Probability"] = ui->leoScanFloodNoiseProbability->text().toDouble();
      njson["SigmaDouble"] = ui->ledScanFloodNoiseOffset->text().toDouble();
      QJsonArray njsonArr;
      for (int iChan=0; iChan<ScanFloodNoise.size(); iChan++)
        {
          QJsonObject chanjson;
          chanjson["Active"] = ScanFloodNoise[iChan].Active;
          chanjson["Description"] = ScanFloodNoise[iChan].Description;
          chanjson["Probability"] = ScanFloodNoise[iChan].Probability;
          chanjson["AverageValue"] = ScanFloodNoise[iChan].AverageValue;
          chanjson["Spread"] = ScanFloodNoise[iChan].Spread;
          njsonArr.append(chanjson);
        }
      njson["NoiseArray"] = njsonArr;
  //  }
  json["BadEventOptions"] = njson;

  if (SimMode == 0 || fVerbose)
    { //Single position options
      QJsonObject spjson;
      spjson["SingleX"] = ui->ledSingleX->text().toDouble();
      spjson["SingleY"] = ui->ledSingleY->text().toDouble();
      spjson["SingleZ"] = ui->ledSingleZ->text().toDouble();
      json["SinglePositionOptions"] = spjson;
    }
  if (SimMode == 1 || fVerbose)
    {
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
    }
  if (SimMode == 2 || fVerbose)
    { //Flood options
      QJsonObject fjson;
      fjson["Nodes"] = ui->sbScanFloodNodes->value();
      int Shape = ui->cobScanFloodShape->currentIndex();
      fjson["Shape"] = Shape;
      //if (Shape == 0)
      //  { //Square
          fjson["Xfrom"] = ui->ledScanFloodXfrom->text().toDouble();
          fjson["Xto"] = ui->ledScanFloodXto->text().toDouble();
          fjson["Yfrom"] = ui->ledScanFloodYfrom->text().toDouble();
          fjson["Yto"] = ui->ledScanFloodYto->text().toDouble();
      //  }
     //else
     //   { //Ring/circle
          fjson["CenterX"] = ui->ledScanFlood_CenterX->text().toDouble();
          fjson["CenterY"] = ui->ledScanFlood_CenterY->text().toDouble();
          fjson["DiameterOut"] = ui->ledScanFlood_Radius->text().toDouble();
          fjson["DiameterIn"] = ui->ledScanFlood_Radius0->text().toDouble();
     //   }
      int Zoption = ui->cobScanFloodZtype->currentIndex();
      fjson["Zoption"] = Zoption;
      //if (Zoption == 0)
      //  { //fixed
          fjson["Zfixed"] = ui->ledScanFloodZ->text().toDouble();
      //  }
      //else
      //  { //range
          fjson["Zfrom"] = ui->ledScanFloodZfrom->text().toDouble();
          fjson["Zto"] = ui->ledScanFloodZto->text().toDouble();
      //  }
      json["FloodOptions"] = fjson;
    }
  if (SimMode == 3 || fVerbose)
    { // custom nodes
      QJsonObject njson;
        njson["Script"] = NodesScript;
        QJsonArray arr;
        for (int i=0; i<CustomScanNodes.size(); i++)
        {
          QJsonArray el;
          el << CustomScanNodes[i]->x() << CustomScanNodes[i]->y() << CustomScanNodes[i]->z();
          arr.append(el);
        }
      njson["Nodes"] = arr;

      json["CustomNodesOptions"] = njson;
    }
  if (SimMode > 3 ) qWarning() << "Unknown sim mode!";

  //adding to master json
  jsonMaster["PointSourcesConfig"] = json;
}

void MainWindow::PointSource_InitTabWidget()
{
  //table inits
  NoiseTableLocked = true;

  ui->tabwidScanFlood->clearContents();
  //ui->tabwidScanFlood->setShowGrid(false);
  ui->tabwidScanFlood->verticalHeader()->setVisible(false);

  ScanFloodNoise.resize(0);
  ScanFloodStructure tmp;
  tmp.Description = "Noise signal in one PM";
  NoiseTypeDescriptions << tmp.Description;
  tmp.Active = false;
  tmp.Probability = 0.01;
  tmp.AverageValue = 100.0;
  tmp.Spread = 40.0;
  ScanFloodNoise.append(tmp);

  tmp.Description = "Noise signal in one PM\n(over a normal event)";
  NoiseTypeDescriptions << tmp.Description;
  tmp.Active = false;
  tmp.Probability = 0.01;
  tmp.AverageValue = 100.0;
  tmp.Spread = 40.0;
  ScanFloodNoise.append(tmp);

  tmp.Description = "Noise signal in all PMs";
  NoiseTypeDescriptions << tmp.Description;
  tmp.Active = false;
  tmp.Probability = 0.01;
  tmp.AverageValue = 100.0;
  tmp.Spread = 40.0;
  ScanFloodNoise.append(tmp);

  tmp.Description = "Noise signal in all PMs\n(over a normal event)";
  NoiseTypeDescriptions << tmp.Description;
  tmp.Active = false;
  tmp.Probability = 0.01;
  tmp.AverageValue = 100.0;
  tmp.Spread = 40.0;
  ScanFloodNoise.append(tmp);

  tmp.Description = "Double event\n(two independent)";
  NoiseTypeDescriptions << tmp.Description;
  tmp.Active = false;
  tmp.Probability = 0.01;
  tmp.AverageValue = 0;
  tmp.Spread = 0;
  ScanFloodNoise.append(tmp);

  tmp.Description = "Double event\n(# of photons is shared)";
  NoiseTypeDescriptions << tmp.Description;
  tmp.Active = false;
  tmp.Probability = 0.01;
  tmp.AverageValue = 0.5;
  tmp.Spread = 0.25;
  ScanFloodNoise.append(tmp);

  int rows = ScanFloodNoise.size();
  ui->tabwidScanFlood->setRowCount(rows);

  for (int j=0; j<rows; j++)
    {

      QTableWidgetItem *item = new QTableWidgetItem();
      if (ScanFloodNoise[j].Active) item->setCheckState(Qt::Checked);
      else item->setCheckState(Qt::Unchecked);
      //item->setText("");
      ui->tabwidScanFlood->setItem(j, 0, item);
      ui->tabwidScanFlood->item(j,0)->setTextAlignment(Qt::AlignHCenter);

      //alternative:
      // ui->tabwidScanFlood->setCellWidget(j,0,new QCheckBox(""));
      //but in this way one cannot set alignment :(

      ui->tabwidScanFlood->setItem(j, 1, new QTableWidgetItem(ScanFloodNoise[j].Description));
      ui->tabwidScanFlood->item(j,1)->setTextAlignment(Qt::AlignHCenter);
      ui->tabwidScanFlood->item(j,1)->setFlags(ui->tabwidScanFlood->item(j,1)->flags() ^ Qt::ItemIsEditable);

      QString str;
      str.setNum(ScanFloodNoise[j].Probability);
      ui->tabwidScanFlood->setItem(j, 2, new QTableWidgetItem(str));
      ui->tabwidScanFlood->item(j,2)->setTextAlignment(Qt::AlignHCenter);

      str.setNum(ScanFloodNoise[j].AverageValue);
      ui->tabwidScanFlood->setItem(j, 3, new QTableWidgetItem(str));
      ui->tabwidScanFlood->item(j,3)->setTextAlignment(Qt::AlignHCenter);

      str.setNum(ScanFloodNoise[j].Spread);
      ui->tabwidScanFlood->setItem(j, 4, new QTableWidgetItem(str));
      ui->tabwidScanFlood->item(j,4)->setTextAlignment(Qt::AlignHCenter);
    }

  ui->tabwidScanFlood->resizeColumnsToContents();
  ui->tabwidScanFlood->resizeRowsToContents();
  NoiseTableLocked = false; 
}

void MainWindow::PointSource_UpdateTabWidget()
{
   NoiseTableLocked = true;

   int rows = ScanFloodNoise.size();
   for (int j=0; j<rows; j++)
     {
       if (ScanFloodNoise[j].Active) ui->tabwidScanFlood->item(j, 0)->setCheckState(Qt::Checked);
       else ui->tabwidScanFlood->item(j, 0)->setCheckState(Qt::Unchecked);

       QString str;
       str.setNum(ScanFloodNoise[j].Probability);
       ui->tabwidScanFlood->item(j, 2)->setText(str);

       str.setNum(ScanFloodNoise[j].AverageValue);
       ui->tabwidScanFlood->item(j, 3)->setText(str);

       str.setNum(ScanFloodNoise[j].Spread);
       ui->tabwidScanFlood->item(j, 4)->setText(str);
     }

   NoiseTableLocked = false;
}

void MainWindow::PointSource_ReadTabWidget()
{
   ScanFloodNoiseProbability  = 0;
   int rows = ScanFloodNoise.size();
   for (int j=0; j<rows; j++)
     {
       bool Active = (ui->tabwidScanFlood->item(j, 0)->checkState() == Qt::Checked);
       ScanFloodNoise[j].Active = Active;

       //data were already validated!
       ScanFloodNoise[j].Probability = ui->tabwidScanFlood->item(j, 2)->text().toDouble();
       ScanFloodNoise[j].AverageValue = ui->tabwidScanFlood->item(j, 3)->text().toDouble();
       ScanFloodNoise[j].Spread = ui->tabwidScanFlood->item(j, 4)->text().toDouble();

       if (Active) ScanFloodNoiseProbability += ScanFloodNoise[j].Probability;
     }

   QString str;
   str.setNum(ScanFloodNoiseProbability);
   ui->leoScanFloodNoiseProbability->setText(str);
}

void MainWindow::on_tabwidScanFlood_cellChanged(int row, int column)
{
   if (NoiseTableLocked) return; //do not update during filling with data!

   if (column > 1)
     { //here input should be double data
       bool ok;
       QString str = ui->tabwidScanFlood->item(row,column)->text();
       double newVal = str.toDouble(&ok);
//       qDebug()<<str<<newVal;
       if (!ok)
         {
           message("Input error!", this);
           MainWindow::PointSource_UpdateTabWidget();
           return;
         }

       if (column == 2)
         if (newVal<0 || newVal>1)
         {
           message("Error: Value should be in the range from 0 to 1", this);
           MainWindow::PointSource_UpdateTabWidget();
           return;
         }
     }

   MainWindow::PointSource_ReadTabWidget();
   MainWindow::on_pbUpdateSimConfig_clicked();
}
