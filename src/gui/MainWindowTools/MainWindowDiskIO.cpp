//ANTS2 modules and windows
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "amaterialparticlecolection.h"
#include "pms.h"
#include "eventsdataclass.h"
#include "materialinspectorwindow.h"
#include "reconstructionwindow.h"
#include "outputwindow.h"
#include "jsonparser.h"
#include "detectorclass.h"
#include "pmtypeclass.h"
#include "geometrywindowclass.h"
#include "globalsettingsclass.h"
#include "asandwich.h"
#include "ajsontools.h"
#include "aenergydepositioncell.h"
#include "afiletools.h"
#include "amessage.h"
#include "aparticleonstack.h"
#include "aconfiguration.h"
#include "tmpobjhubclass.h"
#include "apmgroupsmanager.h"

//Qt
#include <QMessageBox>
#include <QDebug>
#include <QFileDialog>
#include <QFile>
#include <QVector3D>

//Root
#include "TTree.h"
#include "TFile.h"
#include "TGeoManager.h"
#include "TH1I.h"

/*
void MainWindow::SavePhElToSignalData(QString fileName)
{
  QFile outputFile(fileName);
  outputFile.open(QIODevice::WriteOnly);
  if(!outputFile.isOpen())
    {
          message("Unable to open file " +fileName+ " for writing!", this);
          return;
     }

  QTextStream outStream(&outputFile);
  QFileInfo QF(fileName);

  for (int ipm = 0; ipm<PMs->count(); ipm++)
    {
     // outStream<<ipm<<" ";
      int mode = PMs->getSPePHSmode(ipm);
      outStream<<mode<<" ";
      switch (mode)
        {
        case 0:
          outStream<<PMs->getAverageSignalPerPhotoelectron(ipm);
          break;
        case 1:
          outStream<<PMs->getAverageSignalPerPhotoelectron(ipm)<<" ";
          outStream<<PMs->getSPePHSsigma(ipm);
          break;
        case 2:
          outStream<<PMs->getAverageSignalPerPhotoelectron(ipm)<<" ";
          outStream<<PMs->getSPePHSshape(ipm);
          break;
        case 3:
          {
            QString str;
            str.setNum(ipm);
            outStream<<"SPePHS_"+str+".dat";
            SaveDoubleVectorsToFile(QF.path()+"/"+"SPePHS_"+str+".dat", PMs->getSPePHS_x(ipm), PMs->getSPePHS(ipm));
          }
        }
      outStream<<"\n";
    }
  outputFile.close();
}

void MainWindow::LoadPhElToSignalData(QString fileName)
{
  QFile file(fileName);
  if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
      message("Could not open: "+fileName, this);
      return;
    }

  QFileInfo QF(fileName);
  QTextStream in(&file);
  QRegExp rx("(\\ |\\,|\\t)"); //separators: ' ' or ',' or '\t'

  int ipm = -1;
  while(!in.atEnd())
       {
          QString line = in.readLine();
          QStringList fields = line.split(rx, QString::SkipEmptyParts);

 //         qDebug()<<"line: "<<line;
 //         qDebug()<<"fields="<<fields.count();
          if (fields.count()<2) continue;

          int mode = fields[0].toInt();
          if ( (mode==1 || mode==2) && fields.count()<3) continue;
 //         qDebug()<<"adding these data";
          ipm++;
          if (ipm>PMs->count()) continue; //protection, not using break to compare with actual numPMs below

          PMs->setSPePHSmode(ipm, mode);
          switch (mode)
            {
            case 0:
              {
              double mean = fields[1].toDouble();
              PMs->setAverageSignalPerPhotoelectron(ipm, mean);
              break;
              }
            case 1:
              {
              double mean = fields[1].toDouble();
              PMs->setAverageSignalPerPhotoelectron(ipm, mean);
              double sigma = fields[2].toDouble();
              PMs->setSPePHSsigma(ipm, sigma);
              break;
              }
            case 2:
              {
              double mean = fields[1].toDouble();
              PMs->setAverageSignalPerPhotoelectron(ipm, mean);
              double shape = fields[2].toDouble();
              PMs->setSPePHSshape(ipm, shape);
              break;
              }
            case 3:
              {
                QString name = fields[1];
                QVector<double> x, y;
                int error = LoadDoubleVectorsFromFile(QF.path()+"/"+name, &x, &y);
                if (error == 0)
                  {
                    PMs->setElChanSPePHS(ipm, &x, &y);
                  }
              }
            }
        }
   file.close();

   ipm++; //since started from 0
   if (ipm != PMs->count())
   {
       QString str;
       str.setNum(ipm);
       message("Warning: wrong number of PMs (" +str+ ") in the file:"+fileName, this);
   }
}
*/

int MainWindow::LoadAreaResponse(QString fileName, QVector<QVector<double> >* tmp, double* xStep, double* yStep)
{
  QFile file(fileName);
  if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
      message("failed to open file "+fileName, this);
      return 1;
    }

  QTextStream in(&file);
  QRegExp rx("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'

  QString line = in.readLine();
  QStringList fields = line.split(rx);

  if (fields[0].compare("PM-AreaResponse"))
    {
      message("Unrecognized format!", this);
      return 2;
    }
  bool ok1, ok2, ok3, ok4;
  int xNum = fields[1].toInt(&ok1);
  *xStep = fields[2].toDouble(&ok2);  //*** potential problem with decimal separator!
  int yNum = fields[3].toInt(&ok3);
  *yStep = fields[4].toDouble(&ok4);
  if (!ok1 || !ok2 || !ok3 || !ok4)
    {
      message("Unrecognized format!", this);
      return 2;
    }

  qDebug()<<xNum<<*xStep<<yNum<<*yStep;
  //QVector< QVector <double>> tmp;
  tmp->resize(xNum);
  for (int i=0; i<xNum; i++) (*tmp)[i].resize(yNum);

  for (int iY = 0; iY<yNum; iY++)
       {
          if (in.atEnd())
            {
              message("Unexpected end of file reached!", this);
              file.close();
              return 100;
            }
          line = in.readLine();
          fields = line.split(rx, QString::SkipEmptyParts);
          if (fields.size() != xNum)
            {
              message("Unexpected end of line reached!", this);
              file.close();
              return 110;
            }

          for (int iX=0; iX < xNum; iX++)
            {
              double val = fields[iX].toDouble();
              if (val<0 || val>1.0)
                {
                  message("response values should be in range from 0 to 1.0!", this);
                  file.close();
                  return 120;
                }
              (*tmp)[iX][yNum-1-iY] = val; //x: -to+ y: +to-
            }
        }

   file.close();
   return 0;
}

int MainWindow::LoadSPePHSfile(QString fileName, QVector<double>* SPePHS_x, QVector<double>* SPePHS) //0 - no errors
{
  int error = LoadDoubleVectorsFromFile(fileName, SPePHS_x, SPePHS);

  if (error != 0) return error; //message is in the loader function

  double sum = 0;
  for (int i=0; i<SPePHS->size(); i++) sum+= SPePHS->at(i);

  if (TMath::Abs(sum) < 0.0000001)
    {
      message("Sum of all probabilities cannot be zero!", this);
      return 10;
    }

  return error;
}

int MainWindow::LoadPMsignals(QString fileName)
{
  ui->prbarLoadData->setValue(0);
  int loaded = EventsDataHub->loadEventsFromTxtFile(fileName, Config->JSON, Detector->PMs);
  if (loaded<0) message(EventsDataHub->ErrorString, this);
  return loaded;
}

void MainWindow::updateLoaded(int events, int progress)
{
  ui->leoLoadedEvents->setText(QString::number(events));
  ui->prbarLoadData->setValue(progress);
}

int MainWindow::LoadSimulationDataFromTree(QString fileName, int numEvents)
{    
  int numEv = EventsDataHub->loadSimulatedEventsFromTree(fileName, Detector->PMs, numEvents);

  ui->leoTotalLoadedEvents->setText(QString::number(EventsDataHub->Events.size()));
  if (numEv != -1)
    {
      ui->lwLoadedSims->addItem(fileName);
      LoadedTreeFiles.append(fileName);
      ui->leoLoadedEvents->setText(QString::number(numEv));
    }
  else
    {
      message(EventsDataHub->ErrorString, this);
      ui->leoLoadedEvents->setText("0");
    }
  return numEv;
}

void MainWindow::LoadScanPhotonDistribution(QString fileName)
{
  QVector<double> x, y;
  int error = LoadDoubleVectorsFromFile(fileName, &x, &y);
  if (error>0) return;

  if (histScan) delete histScan;
  int size = x.size();
  double* xx = new double [size];
  for (int i = 0; i<size; i++) xx[i]=x[i];
  histScan = new TH1I("histPhotDistr","", size-1, xx);
  for (int j = 1; j<size+1; j++)  histScan->SetBinContent(j, y[j-1]);
  histScan->GetIntegral(); //to make thread safe
  histScan->SetXTitle("Number of generated photons");
  histScan->SetYTitle("Probability");

  ui->pbScanDistrShow->setEnabled(true);
  ui->pbScanDistrDelete->setEnabled(true);
}

void MainWindow::ExportDeposition(QFile &outputFile)
{
  if(!outputFile.isOpen() || !outputFile.isWritable())
          return;

  QTextStream outStream(&outputFile);

  outStream << ">Materials:\n";
  for (int i=0; i<MpCollection->countMaterials(); i++) outStream << "> " << i << " " << (*MpCollection)[i]->name << "\n";
  outStream << ">Particles:\n";
  //for (int i=0; i<Detector->ParticleCollection.size(); i++) outStream << "> " << i << " " << Detector->ParticleCollection[i]->ParticleName << "\n";
  for (int i=0; i<Detector->MpCollection->countParticles(); i++) outStream << "> " << i << " " << Detector->MpCollection->getParticleName(i) << "\n";
  outStream << ">\n";
  outStream << ">Event# particle# particleId MaterialId x[mm] y[mm] z[mm] CellLength[mm] time dE[keV]\n";

  for (int i=0; i<EnergyVector.size(); i++)
    {
      outStream << EnergyVector[i]->eventId
                << " " << EnergyVector[i]->index
                << " " << EnergyVector[i]->ParticleId
                << " " << EnergyVector[i]->MaterialId
                << " " << EnergyVector[i]->r[0]
                << " " << EnergyVector[i]->r[1]
                << " " << EnergyVector[i]->r[2]
                << " " << EnergyVector[i]->cellLength
                << " " << EnergyVector[i]->time
                << " " << EnergyVector[i]->dE
                <<"\n";
    }
}

void MainWindow::ImportDeposition(QFile &file)
{
  QTextStream in(&file);

  MainWindow::ClearData();

  QRegExp rx("(\\ |\\,|\\:|\\t)"); //RegEx for ' ' or ',' or ':' or '\t' //separators
  while(!in.atEnd())
      {
        QString line = in.readLine();
        QStringList fields = line.split(rx, QString::SkipEmptyParts);
        if (fields.size() !=10) continue;

        AEnergyDepositionCell* cell = new AEnergyDepositionCell();

        bool ok, flagOK = true;
        int maxMatIndex = MpCollection->countMaterials()-1;
        cell->eventId = fields[0].toDouble(&ok);
        if (!ok) flagOK = false;
        cell->index = fields[1].toDouble(&ok);
        if (!ok) flagOK = false;
        cell->ParticleId = fields[2].toDouble(&ok);
        if (!ok) flagOK = false;
        cell->MaterialId = fields[3].toDouble(&ok);
        if (!ok) flagOK = false;
        if (cell->MaterialId > maxMatIndex)
          {
            message("Material index is out of range. Retry with 'Reassign material index' option", this);
            delete cell;
            MainWindow::ClearData();
            return;
          }
        cell->r[0] = fields[4].toDouble(&ok);
        if (!ok) flagOK = false;
        cell->r[1] = fields[5].toDouble(&ok);
        if (!ok) flagOK = false;
        cell->r[2] = fields[6].toDouble(&ok);
        if (!ok) flagOK = false;
        cell->cellLength = fields[7].toDouble(&ok);
        if (!ok) flagOK = false;
        cell->time = fields[8].toDouble(&ok);
        if (!ok) flagOK = false;
        cell->dE = fields[9].toDouble(&ok);
        if (!ok) flagOK = false;

        if (ui->cbReassignMatIndex->isChecked())
          {
            Detector->GeoManager->SetCurrentPoint(cell->r);
            Detector->GeoManager->FindNode();
            cell->MaterialId = Detector->GeoManager->GetCurrentVolume()->GetMaterial()->GetIndex();
          }

        if (flagOK) EnergyVector.append(cell);
        else delete cell;
    }

  if (EnergyVector.isEmpty()) message("Error: no data were loaded!", this);
  else ui->pbGenerateLight->setEnabled(true);
}

void MainWindow::LoadDummyPMs(QString DFile)
{
  QFile file(DFile);

  if(!file.open(QIODevice::ReadOnly | QFile::Text)) message("Error while opening Dummy PMs file!"+file.fileName()+"\n"+file.errorString(), this);
  else
    {
      //loading
       QTextStream in(&file);
       QRegExp rx("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'
       while(!in.atEnd())
         {
              QString line = in.readLine();
              QStringList fields = line.split(rx, QString::SkipEmptyParts);            
              if (fields.size() == 8)
                {
                  PMdummyStructure dpm;
                  dpm.PMtype = fields[0].toInt();
                  dpm.UpperLower = fields[1].toInt();
                  dpm.r[0] = fields[2].toDouble();
                  dpm.r[1] = fields[3].toDouble();
                  dpm.r[2] = fields[4].toDouble();
                  dpm.Angle[0] = fields[5].toDouble();
                  dpm.Angle[1] = fields[6].toDouble();
                  dpm.Angle[2] = fields[7].toDouble();
                  Detector->PMdummies.append(dpm);
                }
         }
       file.close();      
    }
}

void MainWindow::on_pbLoadPMcenters_clicked()
{
  int reg = ui->cobPMarrayRegularity->currentIndex();

  QString str;
  int ul = ui->cobUpperLowerPMs->currentIndex();
  if (ul == 0) str = "upper PMs"; else str = "lower PMs";

  if (reg == 1) str = "Load PM center positions (X,Y) for "+str;
  else str = "Load PM center positions (X,Y,Z) for "+str;

  QString fileName = QFileDialog::getOpenFileName(this, str, GlobSet->LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*.*)");
  if (fileName.isEmpty()) return;
  GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();

  APmArrayData *PMar = &Detector->PMarrays[ul];
  PMar->PositionsAnglesTypes.clear();
  if (reg == 1)
    { //2D - autoz
      QVector<double> x, y;
      LoadDoubleVectorsFromFile(fileName, &x, &y);
      for (int i=0; i<x.size(); i++)
        PMar->PositionsAnglesTypes.append(APmPosAngTypeRecord(x[i], y[i],0, 0,0,0, 0));
    }
   else //reg = 2
    { //3D - custom
      QVector<double> x, y, z;
      LoadDoubleVectorsFromFile(fileName, &x, &y, &z);
      for (int i=0; i<x.size(); i++)
        PMar->PositionsAnglesTypes.append(APmPosAngTypeRecord(x[i], y[i], z[i], 0,0,0, 0));
    }
  MainWindow::ReconstructDetector();
  MainWindow::ShowPMcount();
  MainWindow::ClearData();
  Owindow->RefreshData();
}

void MainWindow::on_pbSavePMcenters_clicked()
{
    int A_Reg = ui->cobPMarrayRegularity->currentIndex();

    QString str;
    int ul = ui->cobUpperLowerPMs->currentIndex();
    str = ul == 0 ? "upper PMs" : "lower PMs";

    if (A_Reg == 1) str = "Save PM center positions (X,Y) for "+str;
    else str = "Save PM center positions (X,Y,Z) for "+str;

    QString fileName = QFileDialog::getSaveFileName(this, str, GlobSet->LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*.*)");
    if (fileName.isEmpty()) return;

    GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();
    QFileInfo file(fileName);
    if(file.suffix().isEmpty()) fileName += ".dat";

    QFile outFile(fileName);
    outFile.open(QIODevice::WriteOnly);
    if(!outFile.isOpen())
    {
        QMessageBox::information(this, "I/O Error", "Unable to open file " +fileName+ " for writing!");
        return;
    }
    QTextStream outStream(&outFile);

    if (A_Reg == 1)
    {
        //2D
        for (int i=0; i<PMs->count(); i++)
            if (PMs->at(i).upperLower == ul)
                PMs->at(i).saveCoords2D(outStream);
    }
    else if (A_Reg == 2)
    {
        //3D
        for (int i=0; i<PMs->count(); i++)
            if (PMs->at(i).upperLower == ul)
                PMs->at(i).saveCoords(outStream);
    }
}

void MainWindow::SavePreprocessingAddMulti(QString fileName)
{
  QVector<double> ii, PreprocessingAdd, PreprocessingMultiply;
  for (int i=0; i<PMs->count(); i++)
    {
      ii.append(i);
      PreprocessingAdd.append(PMs->at(i).PreprocessingAdd);
      PreprocessingMultiply.append(PMs->at(i).PreprocessingMultiply);
    }
  ii.append(PMs->count());
  PreprocessingAdd.append(TmpHub->PreEnAdd);
  PreprocessingMultiply.append(TmpHub->PreEnMulti);
  SaveDoubleVectorsToFile(fileName, &ii, &PreprocessingAdd, &PreprocessingMultiply);
}

void MainWindow::LoadPreprocessingAddMulti(QString filename)
{
  QVector<double> ii, add, multiply;
  LoadDoubleVectorsFromFile(filename, &ii, &add, &multiply);

  if (add.size() < PMs->count())
    {
      message("Error: data do not exist for all PMs!", this);
      return;
    }

  if (add.size() < PMs->count()+1  && EventsDataHub->fLoadedEventsHaveEnergyInfo)
    {
      message("Loaded data do not have energy channel, extending with add:0, multiply:1.0!", this);
      add.append(0);
      multiply.append(1.0);
    }

  for (int i=0; i<PMs->count(); i++)
    {
      PMs->at(i).PreprocessingAdd = add[i];
      PMs->at(i).PreprocessingMultiply = multiply[i];
    }
  if (EventsDataHub->fLoadedEventsHaveEnergyInfo)
    {
      TmpHub->PreEnAdd = add.last();
      TmpHub->PreEnMulti = multiply.last();
    }
  MainWindow::on_pbUpdatePreprocessingSettings_clicked();

  MainWindow::on_sbPreprocessigPMnumber_valueChanged(ui->sbPreprocessigPMnumber->value());
}
