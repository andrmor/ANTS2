#ifdef __USE_ANTS_PYTHON__
  #include "Python.h"
  #include "PythonQtConversion.h"
  #include "PythonQt.h"
#endif

//ANTS2 modules
#include "geometrywindowclass.h"
#include "graphwindowclass.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "eventsdataclass.h"
#include "lrfwindow.h"
#include "reconstructionwindow.h"
#include "amaterialparticlecolection.h"
#include "apmhub.h"
#include "materialinspectorwindow.h"
#include "outputwindow.h"
#include "guiutils.h"
#include "windownavigatorclass.h"
#include "exampleswindow.h"
#include "detectoraddonswindow.h"
#include "checkupwindowclass.h"
#include "gainevaluatorwindowclass.h"
#include "credits.h"
#include "detectorclass.h"
#include "asimulationmanager.h"
#include "aparticlesourcesimulator.h"
#include "areconstructionmanager.h"
#include "apmtype.h"
#include "globalsettingswindowclass.h"
#include "aglobalsettings.h"
#include "aopticaloverride.h"
#include "phscatclaudiomodel.h"
#include "scatteronmetal.h"
#include "slabdelegate.h"
#include "asandwich.h"
#include "ajsontools.h"
#include "dotstgeostruct.h"
#include "ageomarkerclass.h"
#include "atrackrecords.h"
#include "afiletools.h"
#include "amessage.h"
#include "acommonfunctions.h"
#include "aenergydepositioncell.h"
#include "tmpobjhubclass.h"
#include "localscriptinterfaces.h"
#include "apreprocessingsettings.h"
#include "aconfiguration.h"
#include "ascriptwindow.h"
#include "gui/alrfwindow.h"
#include "acustomrandomsampling.h"
#include "asourceparticlegenerator.h"
#include "ajavascriptmanager.h"
#include "ascriptwindow.h"
#include "aremotewindow.h"
#include "aopticaloverridedialog.h"
#include "aparticlesourcerecord.h" //---

//Qt
#include <QDebug>
#include <QMessageBox>
#include <QTableWidget>
#include <QFileDialog>
#include <QTimer>
#include <QtWidgets>
#include <QDate>
#include <QPalette>

//Root
#include "TApplication.h"
#include "TGeoMatrix.h"
#include "TSystem.h"
#include "TVirtualGeoTrack.h"//TGeoTrack
#include "TH1D.h"
#include "TH1I.h"
#include "TH2D.h"
#include "TGraph.h"
#include "TGraph2D.h"
#include "TColor.h"
#include "TROOT.h"
#include "TRandom2.h"
#include "TGeoTrack.h"
#include "TGeoManager.h"

#ifdef ANTS_FANN
#include "neuralnetworkswindow.h"
#endif

#ifdef ANTS_FLANN
#include "nnmoduleclass.h"
#endif

// See MainWindowInits.cpp for the constructor!

MainWindow::~MainWindow()
{
    qDebug()<<"<Staring destructor for MainWindow";
    delete histSecScint;
    delete histScan;

    qDebug() << "<-Clearing containers with dynamic objects";
    clearGeoMarkers();
    clearEnergyVector();

    qDebug() << "<-Deleting ui";
    delete ui;
    qDebug() << "<Main window custom destructor finished";
}

void MainWindow::onBusyOn()
{
  ui->menuBar->setEnabled(false);

  ui->tabConfig->setEnabled(false);
  ui->tabExpdata->setEnabled(false);
  ui->tabTests->setEnabled(false);
  ui->twOption->setEnabled(false);
  ui->twSourcePhotonsParticles->setEnabled(false);
  ui->fExportImport->setEnabled(false);
  ui->fSaveExport->setEnabled(false);
}

void MainWindow::onBusyOff()
{
  ui->menuBar->setEnabled(true);

  ui->tabConfig->setEnabled(true);
  ui->tabExpdata->setEnabled(true);
  ui->tabTests->setEnabled(true);
  ui->twOption->setEnabled(true);
  ui->twSourcePhotonsParticles->setEnabled(true); 
  ui->fExportImport->setEnabled(true);
  ui->fSaveExport->setEnabled(true);
  ui->pbStopScan->setChecked(false); //in case stop was triggered
  ui->pbStopScan->setEnabled(false);
}

void MainWindow::startRootUpdate()
{
  if (RootUpdateTimer) RootUpdateTimer->start();
}

void MainWindow::stopRootUpdate()
{
  if (RootUpdateTimer) RootUpdateTimer->stop();
}

static bool DontAskAgainPlease = false;
void MainWindow::ClearData()
{   
   //   qDebug() << "---Before clear";
   if (msBox) return; //not yet confirmed clear status - this was called due to focus out.
   if (fSimDataNotSaved && !DontAskAgainPlease)
     {
       msBox = new QMessageBox();
       msBox->setText("Potential data loss!");
       msBox->setInformativeText("Unsaved simulation data are about to be deleted!\nSave simulation data?");
       QPushButton *saveButton = msBox->addButton("Save", QMessageBox::YesRole);
       msBox->addButton("Discard", QMessageBox::ActionRole);
       QPushButton *daaButton = msBox->addButton("Don't ask", QMessageBox::ActionRole);
       msBox->setDefaultButton(QMessageBox::Save);
       msBox->exec();

       fSimDataNotSaved = false;
       if (msBox->clickedButton() == saveButton) SaveSimulationDataTree();
       else if (msBox->clickedButton() == daaButton) DontAskAgainPlease = true;

       delete msBox;
       msBox = 0;
     }
   fSimDataNotSaved = false;

   clearEnergyVector();
   EventsDataHub->clear();
   //gui updated will be automatically triggered
}

void MainWindow::onRequestUpdateGuiForClearData()
{
    //  qDebug() << ">>> Main window: OnClear signal received";
    clearGeoMarkers();
    clearEnergyVector();
    ui->leoLoadedEvents->setText("");
    ui->leoTotalLoadedEvents->setText("");
    ui->lwLoadedSims->clear();
    ui->pbGenerateLight->setEnabled(false);
    Owindow->SiPMpixels.clear();
    Owindow->RefreshData();
    WindowNavigator->ResetAllProgressBars();

    ui->fReloadRequired->setVisible(false);
    LoadedEventFiles.clear();
    ui->lwLoadedEventsFiles->clear();
    LoadedTreeFiles.clear();
    //  qDebug()  << ">>> Main window: Clear done";
}

void MainWindow::clearEnergyVector()
{
    for (int i=0; i<EnergyVector.size(); i++) delete EnergyVector[i];
    EnergyVector.clear();
}

void MainWindow::closeEvent(QCloseEvent *)
{
   qDebug() << "\n<MainWindow shutdown initiated";
   ShutDown = true;

   /*
   if (ReconstructionManager->isBusy() || !SimulationManager->fFinished)
       if (timesTriedToExit < 6)
       {
           //qDebug() << "<-Reconstruction manager is busy, terminating and trying again in 100us";
           ReconstructionManager->requestStop();
           SimulationManager->StopSimulation();
           qApp->processEvents();
           QThread::usleep(100);
           QTimer::singleShot(100, this, SLOT(close()));
           timesTriedToExit++;
           event->ignore();
           return;
       }
   */

   ui->pbShowColorCoding->setFocus(); //to finish editing whatever QLineEdit the user can be in - they call on_editing_finish

   if (GraphWindow->isBasketOn())
   {
       bool bVis = GraphWindow->isVisible();
       GraphWindow->showNormal();
       GraphWindow->switchOffBasket();
       GraphWindow->setVisible(bVis);
   }

   //if checked, save windows' status
   if (ui->actionSave_Load_windows_status_on_Exit_Init->isChecked())
     {
       qDebug()<<"<Saving position/status of all windows";
       MainWindow::on_actionSave_position_and_stratus_of_all_windows_triggered();
     }

   qDebug() << "<Preparing graph window for shutdown";
   GraphWindow->close();
   GraphWindow->ClearDrawObjects_OnShutDown(); //to avoid any attempts to redraw deleted objects

   //saving ANTS master-configuration file
   qDebug() << "<Saving JavaScripts";
   ScriptWindow->WriteToJson();
   if (PythonScriptWindow)
   {
       qDebug() << "<Saving Python scripts";
       PythonScriptWindow->WriteToJson();
   }
   qDebug() << "<Saving remote servers";
   RemoteWindow->WriteConfig();
   qDebug()<<"<Saving global settings";
   GlobSet.saveANTSconfiguration();

   EventsDataHub->clear();

   qDebug()<<"<Saving ANTS configuration";
   ELwindow->QuickSave(0);

   qDebug() << "<Stopping Root update timer-based cycle";
   RootUpdateTimer->stop();
   disconnect(RootUpdateTimer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
   QThread::msleep(110);
   delete RootUpdateTimer;
   //qDebug()<<"        timer stopped and deleted";

   if (ScriptWindow)
     {
       qDebug()<<"<-Deleting ScriptWindow";
       delete ScriptWindow->parent();
       ScriptWindow = 0;
     }

   if (GenScriptWindow)
     {
       GenScriptWindow->close();
       qDebug() << "<-Deleting local script window";
       delete GenScriptWindow; GenScriptWindow = 0;
     }

#ifdef ANTS_FANN
   if (NNwindow)
     {
       qDebug() << "<-Deleting NeuralNetworksWindow";
       delete NNwindow->parent();
       NNwindow = 0;
     }
#endif

   if (lrfwindow)
   {
       qDebug() << "<-Deleting  lrf Window";
       delete lrfwindow->parent();
       lrfwindow = 0;
   }
   if (GeometryWindow)
     {
       qDebug() << "<-Deleting Geometry Window";
       GeometryWindow->hide();
       delete GeometryWindow->parent();
       GeometryWindow = 0;
     }
   if (GraphWindow)
     {
       qDebug() << "<-Deleting Graph Window";
       GraphWindow->hide();
       delete GraphWindow->parent();
       GraphWindow = 0;
     }   
   if (GlobSetWindow)
     {
       qDebug() << "<-Deleting Settings Window";
       GlobSetWindow->hide();
       delete GlobSetWindow;
       GlobSetWindow = 0;
     }
   if (Rwindow)
     {
       qDebug() << "<-Deleting Reconstruction window";
       Rwindow->close();
       delete Rwindow->parent();
       Rwindow = 0;
     }
   if (Owindow)
     {
       qDebug() << "<-Deleting Output window";
       Owindow->close();
       delete Owindow->parent();
       Owindow = 0;
     }
   if (MIwindow)
     {
       qDebug() << "<-Deleting material inspector window";
       delete MIwindow->parent();
       MIwindow = 0;
     }
   if (ELwindow)
     {
       qDebug() << "<-Deleting examples window";
       ELwindow->close();
       delete ELwindow->parent();
       ELwindow = 0;
     }
   if (DAwindow)
     {
       qDebug() << "<-Deleting detector addon window";
       delete DAwindow;
       DAwindow = 0;
     }
   if (CheckUpWindow)
     {
       qDebug() << "<-Deleting check up window";
       delete CheckUpWindow;
       CheckUpWindow = 0;
     }
   if (RemoteWindow)
   {
       qDebug() << "<-Deleting remote simulation/reconstruction window";
       delete RemoteWindow;
       RemoteWindow = 0;
   }
   //Gain evaluation window is deleted in ReconstructionWindow destructor!

   // should be deleted last
   if (WindowNavigator)
   {
       qDebug() << "<-Deleting WindowNavigator";
       delete WindowNavigator->parent();
       WindowNavigator = 0;
   }
   qDebug() << "<MainWindow close event processing finished";
}

bool MainWindow::event(QEvent *event)
{
   if (!WindowNavigator) return QMainWindow::event(event);

   if (event->type() == QEvent::Hide)
     {
//       qDebug()<<"Main window HIDE event";
       WindowNavigator->HideWindowTriggered("main");
       return true; //stop hadling
     }

   if (event->type() == QEvent::WindowStateChange)
     {
//       qDebug()<<"Main window STATECHANGE event";
//       qDebug()<<"Was requested by navigator:"<<WindowNavigator->isMainChangeExplicitlyRequested();
//       qDebug()<<"Is window currently minimized:"<<this->isMinimized();

       //note: after "minimize" is requested by any source, the WindowStateChange triggered AFTER the minimization

       if (WindowNavigator->isMainChangeExplicitlyRequested())
         {

           if (this->isMinimized()) WindowNavigator->HideWindowTriggered("main");
           else WindowNavigator->ShowWindowTriggered("main");
           return true;
         }
       else
         {
            if (this->isMinimized())
              {
                //qDebug() << "main: minimized, so hiding all other windows";
                WindowNavigator->TriggerHideButton();
                //qDebug() << "main done";
              }
            else
              {
                WindowNavigator->TriggerShowButton();
                if (this->isVisible()) this->activateWindow();
              }
            return true;
         }
     }

   if (event->type() == QEvent::Show)
     {      
//       qDebug()<<"Main window SHOW event";
       WindowNavigator->SetupWindowsTaskbar(); // in effect only once on system start
       WindowNavigator->ShowWindowTriggered("main");
       return true;
     }

   return QMainWindow::event(event);
}

void MainWindow::clearGeoMarkers(int All_Rec_True)
{    
  for (int i=GeoMarkers.size()-1; i>-1; i--)
  {
      switch (All_Rec_True)
      {
        case 1:
          if (GeoMarkers.at(i)->Type == "Recon")
          {
              delete GeoMarkers[i];
              GeoMarkers.remove(i);
          }
          break;
        case 2:
          if (GeoMarkers.at(i)->Type == "Scan")
          {
              delete GeoMarkers[i];
              GeoMarkers.remove(i);
          }
          break;
        case 0:
        default:
          delete GeoMarkers[i];
      }
  }

  if (All_Rec_True == 0) GeoMarkers.clear();
}

void MainWindow::ShowGeoMarkers()
{
  if (!GeoMarkers.isEmpty())
    {
      GeometryWindow->SetAsActiveRootWindow();
      for (int i=0; i<GeoMarkers.size(); i++)
        {
          GeoMarkerClass* gm = GeoMarkers[i];
          //overrides
          if (gm->Type == "Recon" || gm->Type == "Scan" || gm->Type == "Nodes")
            {
              gm->SetMarkerStyle(GeometryWindow->GeoMarkerStyle);
              gm->SetMarkerSize(GeometryWindow->GeoMarkerSize);
            }
          gm->Draw("same");
        }
      GeometryWindow->UpdateRootCanvas();
    }
}

void MainWindow::ShowGraphWindow()
{
  GraphWindow->ShowAndFocus();  
}

void MainWindow::on_pbRebuildDetector_clicked()
{   
  if (DoNotUpdateGeometry) return; //if bulk update in progress
  //world size
  Detector->fWorldSizeFixed = ui->cbFixedTopSize->isChecked();
  if (Detector->fWorldSizeFixed)
  {
    Detector->WorldSizeXY = 0.5*ui->ledTopSizeXY->text().toDouble();
    Detector->WorldSizeZ = 0.5*ui->ledTopSizeZ->text().toDouble();
  }
  MainWindow::ReconstructDetector();
}

// -------materials: visualization--------------
void MainWindow::on_pbRefreshMaterials_clicked()
{
    //remember selected optical override materials - they will be resetted by clear cobs
    int OvFrom = ui->cobMaterialForOverrides->currentIndex();
    int OvTo = ui->cobMaterialTo->currentIndex();

    UpdateMaterialListEdit();
    bool tmpBool = DoNotUpdateGeometry;
    DoNotUpdateGeometry = true;
    //updating material selectors on the Detector tab   
    ui->cobMatPM->clear();
    ui->cobMaterialForOverrides->clear();
    ui->cobMaterialTo->clear();
    ui->cobMaterialForWaveTests->clear();
    const QStringList mn = MpCollection->getListOfMaterialNames();
    ui->cobMatPM->addItems(mn);
    ui->cobMaterialForOverrides->addItems(mn);
    ui->cobMaterialTo->addItems(mn);
    ui->cobMaterialForWaveTests->addItems(mn);

    MIwindow->UpdateActiveMaterials();
    Owindow->UpdateMaterials();

    DoNotUpdateGeometry = tmpBool;

    //restore override material selection
    int numMats = MpCollection->countMaterials();
    if (OvFrom>-1 && OvTo>-1)
        if (OvFrom<numMats && OvTo<numMats)
          {
            ui->cobMaterialForOverrides->setCurrentIndex(OvFrom);
            ui->cobMaterialTo->setCurrentIndex(OvTo);
          }
}

void MainWindow::UpdateMaterialListEdit()
{
  ui->lwMaterials->clear();
  int NumMaterials = MpCollection->countMaterials();

  QString tmpStr, tmpStr2;
  for (int i=0; i<NumMaterials; i++)
   {
    tmpStr2.setNum(i);
    tmpStr=tmpStr2+">  "+(*MpCollection)[i]->name;

    //if there are overrides, add (*)
    bool fFound = false;
    for (int p=0; p<NumMaterials; p++)      
      if ( (*MpCollection)[i]->OpticalOverrides[p] )
        {
          fFound = true;
          break;
        }

    if (fFound) tmpStr += "  (ov)";

    QListWidgetItem* pItem =new QListWidgetItem(tmpStr);
    if (GeometryWindow->isColorByMaterial())
      {
        TColor* rc = gROOT->GetColor(i+1);
        QColor qc = QColor(255*rc->GetRed(), 255*rc->GetGreen(), 255*rc->GetBlue(), 255*rc->GetAlpha());
        pItem->setForeground(qc);
      }
    else pItem->setForeground(Qt::black);   
    ui->lwMaterials->addItem(pItem);
  }
}

void MainWindow::ShowPMcount()
{
  QString str;
  str.setNum(PMs->count());
  ui->labTotPMs->setText(str);
}

void MainWindow::PopulatePMarray(int ul, double z, int istart) // ul= 0 upper, 1 = lower; z - z coordinate; istart - first PM fill be added at this index
{  
  bool hexagonal;
  if (Detector->PMarrays[ul].Packing == 0) hexagonal = false; else hexagonal = true;
  int iX = Detector->PMarrays[ul].NumX;
  int iY = Detector->PMarrays[ul].NumY;
  double CtC = Detector->PMarrays[ul].CenterToCenter;
  double CtCbis = CtC*cos(30.*3.14159/180.);
  int rings = Detector->PMarrays[ul].NumRings;
  bool XbyYarray = !Detector->PMarrays[ul].fUseRings;
  double Psi = 0;

  double x, y;
  double typ = Detector->PMarrays[ul].PMtype;

  int ipos = istart; //where (ipm) insert the next PM

  //populating PM array
  if (XbyYarray)
  { //X by Y type of array  
    if (hexagonal)
     {
      for (int j=iY-1; j>-1; j--)
          for (int i=0; i<iX; i++)
            {
              x = CtC*( -0.5*(iX-1) +i  + 0.5*(j%2));
              y = CtCbis*(-0.5*(iY-1) +j);
              PMs->insert(ipos, ul,x,y,z,Psi,typ);
              ipos++;
            }
     }
    else
     {
       for (int j=iY-1; j>-1; j--)
           for (int i=0; i<iX; i++)
             {
               x = CtC*(-0.5*(iX-1) +i);
               y = CtC*(-0.5*(iY-1) +j);
               PMs->insert(ipos, ul,x,y,z,Psi,typ);
               ipos++;
             }
      }
  }
  else
    { //rings array
      if (hexagonal) {
     //   int nPMs = 1; //0 rings = 1 PM
     //   for (int i=1; i<rings+1; i++) nPMs += 6*i; //every new ring adds 6i PMs

        //qDebug()<<nPMs<<"for rings:"<<rings;
        PMs->insert(ipos, ul,0,0,z, Psi, typ);
        ipos++;

        ///int index=PMs->count();
        //int index = ipos;
        for (int i=1; i<rings+1; i++)
          {
            x = i*CtC;
            y = 0;
            PMs->insert(ipos, ul,x,y,z,Psi,typ);
            ipos++;
            //qDebug()<<"-*"<<PMx[index]<<PMy[index];
            //index++;

            for (int j=1; j<i+1; j++) {  //   /
                x = PMs->X(ipos-1) - 0.5*CtC; //was index
                y = PMs->Y(ipos-1) - CtCbis;
                PMs->insert(ipos, ul,x,y,z,Psi,typ);
                ipos++;
                // qDebug()<<"/"<<PMx[index]<<PMy[index];
                //index++;
            }
            for (int j=1; j<i+1; j++) {   // -
                x = PMs->X(ipos-1) - CtC;
                y = PMs->Y(ipos-1);
                PMs->insert(ipos, ul,x,y,z,Psi,typ);
                ipos++;
                // qDebug()<<"-"<<PMx[index]<<PMy[index];
                //index++;
            }
            for (int j=1; j<i+1; j++) {  // right-down
                x = PMs->X(ipos-1) - 0.5*CtC;
                y = PMs->Y(ipos-1) + CtCbis;
                PMs->insert(ipos, ul,x,y,z,Psi,typ);
                ipos++;
                // qDebug()<<"\\"<<PMx[index]<<PMy[index];
                //index++;
            }
            for (int j=1; j<i+1; j++) {  // /
                x = PMs->X(ipos-1) + 0.5*CtC;
                y = PMs->Y(ipos-1) + CtCbis;
                PMs->insert(ipos, ul,x,y,z,Psi,typ);
                ipos++;
                //qDebug()<<"/"<<PMx[index]<<PMy[index];
                //index++;
            }
            for (int j=1; j<i+1; j++) {   // -
                x = PMs->X(ipos-1) + CtC;
                y = PMs->Y(ipos-1);
                PMs->insert(ipos, ul,x,y,z,Psi,typ);
                ipos++;
                //qDebug()<<"-"<<PMx[index]<<PMy[index];
                //index++;
            }
            for (int j=1; j<i; j++) {  // left-up       //dont do the last step - already positioned PM
                x = PMs->X(ipos-1) + 0.5*CtC;
                y = PMs->Y(ipos-1) - CtCbis;
                PMs->insert(ipos, ul,x,y,z,Psi,typ);
                ipos++;
                //qDebug()<<"\\"<<PMx[index]<<PMy[index];
                //index++;
            }
        }
      }
      else {
         //using the same algorithm as X * Y
         iX = rings*2 + 1;
         iY = rings*2 + 1;

         for (int j=0; j<iY; j++)
             for (int i=0; i<iX; i++) {
                 x = CtC*(-0.5*(iX-1) +i);
                 y = CtC*(-0.5*(iY-1) +j);
                 PMs->insert(ipos, ul,x,y,z,Psi,typ);
                 ipos++;
             }
      }
    }
}

void MainWindow::on_cbXbyYarray_stateChanged(int arg1)
{
        ui->sbNumX->setEnabled(arg1);
        ui->sbNumY->setEnabled(arg1);
}

void MainWindow::on_cbRingsArray_stateChanged(int arg1)
{
     ui->sbPMrings->setEnabled(arg1);
}

void MainWindow::on_pbEditOverride_clicked()
{
    int From = ui->cobMaterialForOverrides->currentIndex();
    int To =   ui->cobMaterialTo->currentIndex();

    AOpticalOverrideDialog* d = new AOpticalOverrideDialog(this, From, To);
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->setWindowModality(Qt::WindowModal);
    QObject::connect(d, &AOpticalOverrideDialog::accepted, this, &MainWindow::onOpticalOverrideDialogAccepted);
    d->show();
}

void MainWindow::onOpticalOverrideDialogAccepted()
{
    ReconstructDetector(true);
    on_pbRefreshOverrides_clicked();
    int i = ui->lwMaterials->currentRow();
    UpdateMaterialListEdit(); //to update (*) status
    ui->lwMaterials->setCurrentRow(i);
}

void MainWindow::on_pbRefreshOverrides_clicked()
{
    ui->lwOverrides->clear();
    int numMat = MpCollection->countMaterials();
    int iMatFrom = ui->cobMaterialForOverrides->currentIndex();

    for (int iMatTo = 0; iMatTo < numMat; iMatTo++)
    {
        const AOpticalOverride* ov = (*MpCollection)[iMatFrom]->OpticalOverrides.at(iMatTo);
        if (!ov) continue;

        QString s = QString("->%1 = ").arg( (*MpCollection)[iMatTo]->name );
        s += ov->getAbbreviation();
        s += ": ";
        s += ov->getReportLine();
        ui->lwOverrides->addItem(s);
    }
}

void MainWindow::on_pbStartMaterialInspector_clicked()
{
    int imat = ui->lwMaterials->currentRow();
    if (imat != -1) MIwindow->SetMaterial(imat);

    MIwindow->showNormal();
    MIwindow->raise();
    MIwindow->activateWindow();   
}

void MainWindow::on_cbSecondAxis_toggled(bool checked)
{
    ui->fScanSecond->setEnabled(checked);
}

void MainWindow::on_cbThirdAxis_toggled(bool checked)
{
    ui->fScanThird->setEnabled(checked);
}

void MainWindow::on_cobPMdeviceType_currentIndexChanged(int index)
{ //on user click, see method on_cobPMdeviceType_activated(const QString &arg1)
  ui->swPMTvsSiPM->setCurrentIndex(index);

  if (index == 0)
    {
      ui->cobPMshape->setEnabled(true);

      ui->labPDE->setText("QE vs. wavelength:");
      ui->labPDEnwr->setText("QE:");
      ui->twPMtypeProperties->setTabText(0, "Quantum Efficiency");
    }
  else
    {
      ui->cobPMshape->setCurrentIndex(0);
      ui->cobPMshape->setEnabled(false);

      ui->labPDE->setText("PDE vs. wavelength:");
      ui->labPDEnwr->setText("PDE:");
      ui->twPMtypeProperties->setTabText(0, "Photon Detection Efficiency");
    }  
}

void MainWindow::on_cobPMdeviceType_activated(const QString &/*arg1*/)
{  //see also method on_cobPMdeviceType_currentIndexChanged(int index)
    MainWindow::on_pbUpdatePMproperties_clicked();
}

void MainWindow::CheckSetMaterial(const QString name, QComboBox* cob, QVector<QString>* vec)
{
    for (int i=0; i<MpCollection->countMaterials(); i++)
    {
        if (!(*MpCollection)[i]->name.compare(name,Qt::CaseSensitive))
        {
            cob->setCurrentIndex(i);
            return;
        }
    }
    //not found
   vec->append(name);
}

void MainWindow::on_cbUPM_toggled(bool checked)
{    
  Detector->PMarrays[0].fActive = checked;
  ToggleUpperLowerPMs();
}

void MainWindow::on_cbLPM_toggled(bool checked)
{
  Detector->PMarrays[1].fActive = checked;
  ToggleUpperLowerPMs();
}

void MainWindow::ToggleUpperLowerPMs()
{  
  //upper-lower selector in arrays is visible only if both lower and upper are enabled
  if (!ui->cbUPM->isChecked() || !ui->cbLPM->isChecked()) ui->fUpperLowerArrays->setVisible(false);
  else ui->fUpperLowerArrays->setVisible(true);

  //autoselect array  
  if (ui->cbUPM->isChecked() && !ui->cbLPM->isChecked()) ui->cobUpperLowerPMs->setCurrentIndex(0);
  else if (!ui->cbUPM->isChecked() && ui->cbLPM->isChecked()) ui->cobUpperLowerPMs->setCurrentIndex(1);

  //force-trigger visualization  ***!!!
  MainWindow::on_cobUpperLowerPMs_currentIndexChanged(ui->cobUpperLowerPMs->currentIndex());

  if (DoNotUpdateGeometry) return; //if it is triggered during load/init hase
  MainWindow::on_pbRebuildDetector_clicked();  //rebuild detector
}

void MainWindow::on_pbRefreshPMArrayData_clicked()
{  
  if (DoNotUpdateGeometry) return;

  int ul = ui->cobUpperLowerPMs->currentIndex();
//  qApp->processEvents();
  if (ui->cobPMarrayRegularity->currentIndex() == 0)
    {
      //Updating regular properties
      Detector->PMarrays[ul].PMtype = ui->sbPMtypeForGroup->value();
      Detector->PMarrays[ul].Packing = ui->cobPacking->currentIndex();
      Detector->PMarrays[ul].CenterToCenter = ui->ledCtC->text().toDouble();
      Detector->PMarrays[ul].NumX = ui->sbNumX->value();
      Detector->PMarrays[ul].NumY = ui->sbNumY->value();
      Detector->PMarrays[ul].NumRings = ui->sbPMrings->value();
      Detector->PMarrays[ul].fUseRings = ui->cbRingsArray->isChecked();
    }  
  Detector->PMarrays[ul].Regularity = ui->cobPMarrayRegularity->currentIndex();

  //if (ui->cobPMarrayRegularity->currentIndex() < 2) MainWindow::RebuildPmArray();
  if (ui->cobPMarrayRegularity->currentIndex() == 1)
    {
      //there will be no automatic update of the base type in the array, have to do it:
      for (int i=0; i<PMs->count(); i++)
        if (PMs->at(i).upperLower == ul) PMs->at(i).type = ui->sbPMtypeForGroup->value();
    }

  //if (geometryRW) MainWindow::on_pbCreateCustomConfiguration_clicked();
  MainWindow::on_pbRebuildDetector_clicked();

  //MainWindow::ClearData();
  Owindow->RefreshData();
}

void MainWindow::on_pbShowPMsArrayRegularData_clicked()
{
   updatePMArrayDataIndication();
}

void MainWindow::updatePMArrayDataIndication()
{
  int i = ui->cobUpperLowerPMs->currentIndex();

  bool tmpBool = DoNotUpdateGeometry;
  DoNotUpdateGeometry = true;

  ui->sbPMtypeForGroup->setValue(Detector->PMarrays[i].PMtype);
  ui->cobPMtypeInArrays->setCurrentIndex(Detector->PMarrays[i].PMtype);
  ui->cobPMarrayRegularity->setCurrentIndex(Detector->PMarrays[i].Regularity);
  if (!BulkUpdate) MainWindow::on_cobPMarrayRegularity_currentIndexChanged(Detector->PMarrays[i].Regularity); //force update
  ui->cobPacking->setCurrentIndex(Detector->PMarrays[i].Packing);
  QString str;
  str.setNum(Detector->PMarrays[i].CenterToCenter, 'g', 4);
  ui->ledCtC->setText(str);
  ui->sbNumX->setValue(Detector->PMarrays[i].NumX);
  ui->sbNumY->setValue(Detector->PMarrays[i].NumY);
  ui->sbPMrings->setValue(Detector->PMarrays[i].NumRings);
  ui->cbRingsArray->setChecked(Detector->PMarrays[i].fUseRings);
  ui->cbXbyYarray->setChecked(!Detector->PMarrays[i].fUseRings);

  DoNotUpdateGeometry = tmpBool;
  MainWindow::ShowPMcount();
}

void MainWindow::on_pbRefreshPMproperties_clicked()
{
    //refresh indication of PM model properties
    int index = ui->sbPMtype->value();
    const APmType *type = PMs->getType(index);

    bool tmpBool = DoNotUpdateGeometry;
    DoNotUpdateGeometry = true;
    ui->lePMtypeName->setText(type->Name);
    ui->cobPMdeviceType->setCurrentIndex(type->SiPM);
    ui->cobMatPM->setCurrentIndex(type->MaterialIndex);
    ui->cobPMshape->setCurrentIndex(type->Shape);
    QString str;

    str.setNum(type->SizeX, 'g', 4);
    ui->ledSizeX->setText(str);
    str.setNum(type->SizeY, 'g', 4);
    ui->ledSizeY->setText(str);
    str.setNum(type->SizeZ, 'g', 4);
    ui->ledSizeZ->setText(str);
    ui->ledSphericalPMAngle->setText( QString::number(type->AngleSphere, 'g', 4) );
    ui->sbSiPMnumx->setValue(type->PixelsX);
    ui->sbSiPMnumy->setValue(type->PixelsY);
    str.setNum(type->PixelsX * type->PixelsY);
    ui->labTotalPixels->setText("("+str+")");
    str.setNum(type->DarkCountRate, 'g', 4);
    ui->ledSiPMdarCountRate->setText(str);
    str.setNum(type->RecoveryTime, 'g', 4);
    ui->ledSiPMrecoveryTime->setText(str);

    //for spherical:
    if (type->Shape == 3) //sphere
        ui->labSphericalPMinfo->setText( QString("%1 / %2").arg(type->getProjectionRadiusSpherical()).arg(2.0*type->getHalfHeightSpherical()) );

    str.setNum(type->EffectivePDE, 'g', 4);
    ui->ledPDE->setText(str);
    ui->pbShowPDE->setEnabled(type->PDE_lambda.size());
    ui->pbShowPDEbinned->setEnabled(ui->cbWaveResolved->isChecked() && type->PDE_lambda.size());
    ui->pbDeletePDE->setEnabled(type->PDE_lambda.size());
    MainWindow::RefreshAngularButtons();
    MainWindow::RefreshAreaButtons();
    str.setNum(type->AngularN1, 'g', 4);
    ui->ledMediumRefrIndex->setText(str);

    //enable/disable control
    int numModels = PMs->countPMtypes();
    bool SeveralModels = (numModels > 1);
    ui->fSelectPMmodel->setVisible(SeveralModels);
    ui->pbRemoveThisPMtype->setVisible(SeveralModels);

    //update cob
    ui->cobPMtypes->clear();
    for (int i=0; i<numModels; i++) ui->cobPMtypes->addItem(PMs->getType(i)->Name);
    ui->cobPMtypes->setCurrentIndex(index);

    DoNotUpdateGeometry = tmpBool;
}

void MainWindow::on_pbUpdatePMproperties_clicked()
{  
   if (DoNotUpdateGeometry) return;

   int index = ui->sbPMtype->value();
   APmType *type = PMs->getType(index);
   type->Name = ui->lePMtypeName->text();

   type->SiPM = ui->cobPMdeviceType->currentIndex();
   type->MaterialIndex = ui->cobMatPM->currentIndex();
   type->Shape = ui->cobPMshape->currentIndex();
   type->SizeX = ui->ledSizeX->text().toDouble();
   type->SizeY = ui->ledSizeY->text().toDouble();
   type->SizeZ = ui->ledSizeZ->text().toDouble();

   type->AngleSphere = ui->ledSphericalPMAngle->text().toDouble();

   type->PixelsX = ui->sbSiPMnumx->value();
   type->PixelsY = ui->sbSiPMnumy->value();
   type->DarkCountRate = ui->ledSiPMdarCountRate->text().toDouble();
   type->RecoveryTime = ui->ledSiPMrecoveryTime->text().toDouble();
   if (type->SiPM) type->Shape = 0; //SiPM - always rectangular
   type->EffectivePDE = ui->ledPDE->text().toDouble();
   QString str;
   str.setNum(type->PixelsX * type->PixelsY);
   ui->labTotalPixels->setText("("+str+")");

   updateCOBsWithPMtypeNames();
   on_pbShowPMsArrayRegularData_clicked(); //in case type name was changed

   ReconstructDetector();
   on_pbUpdateSimConfig_clicked();

   //for indication, update PMs binned properties
   //on_cbAngularSensitive_toggled(ui->cbAngularSensitive->isChecked());
}

void MainWindow::on_sbPMtype_valueChanged(int arg1)
{
  ui->lTypeNumber->setText("#"+QString::number(arg1));

  if (DoNotUpdateGeometry) return;
  if (arg1 > PMs->countPMtypes()-1) ui->sbPMtype->setValue(PMs->countPMtypes()-1);
  MainWindow::on_pbRefreshPMproperties_clicked();
}

void MainWindow::on_sbPMtypeForGroup_valueChanged(int arg1)
{
  if (arg1 > PMs->countPMtypes()-1)
    {
      ui->sbPMtypeForGroup->setValue(PMs->countPMtypes()-1);
      return;
    }
  ui->cobPMtypeInArrays->setCurrentIndex(arg1); //save for cross-call
  //ui->leoPMtypeInArrayTab->setText(PMs->getTypeProperties(arg1)->name);
  MainWindow::on_pbRefreshPMArrayData_clicked();
}

void MainWindow::on_cbXbyYarray_clicked()
{
    ui->cbRingsArray->toggle();
    MainWindow::on_pbRefreshPMArrayData_clicked();
}

void MainWindow::on_cbRingsArray_clicked()
{
    ui->cbXbyYarray->toggle();
    MainWindow::on_pbRefreshPMArrayData_clicked();
}

void MainWindow::on_cobUpperLowerPMs_currentIndexChanged(int index)
{
  if (index < 0) return;

  bool upper;
  if (index == 0) upper = true; else upper = false;
  ui->lineUpperPMs->setVisible(upper);
  ui->lineLowerPMs->setVisible(!upper);
  if (Detector->PMarrays[index].fActive) MainWindow::on_pbShowPMsArrayRegularData_clicked();
}

void MainWindow::on_pbRemoveThisPMtype_clicked()
{
    int itype = ui->sbPMtype->value();
    const QString err = Detector->removePMtype(itype);
    if (!err.isEmpty()) message(err, this);
    else ReconstructDetector();
}

void MainWindow::on_pbAddNewPMtype_clicked()
{
  MainWindow::AddDefaultPMtype();
  ui->sbPMtype->setValue(PMs->countPMtypes()-1); //update of indication is in on_change
}

void MainWindow::AddDefaultPMtype()
{
  //creating a unique name
  QString name = "Type" + QString::number(PMs->countPMtypes());
  bool found;
  do
    {
      found = false;
      for (int i=0; i<PMs->countPMtypes(); i++)
        if (name == PMs->getType(i)->Name)
          {
            found = true;
            break;
          }
      if (found) name += "_1";
    }
  while (found);
  APmType *type = new APmType(name);
  PMs->appendNewPMtype(type);

  //updating all comboboxes with PM type names
  MainWindow::updateCOBsWithPMtypeNames();
}

void MainWindow::updateCOBsWithPMtypeNames()
{
  if (PMs->count() == 0) return;

  int defTypes = PMs->countPMtypes();
  if (defTypes == 0) return; //protection

  bool tmpBool = DoNotUpdateGeometry;
  DoNotUpdateGeometry = true;

  ui->cobPMtypes->clear();
  ui->cobPMtypeInArrays->clear();
  ui->cobPMtypeInExplorers->clear();
  ui->cobPMtypeInArraysForCustom->clear();

  for (int i=0; i<defTypes; i++)
    {
      QString name = PMs->getType(i)->Name;
      ui->cobPMtypes->addItem(name);
      ui->cobPMtypeInArrays->addItem(name);
      ui->cobPMtypeInExplorers->addItem(name);
      ui->cobPMtypeInArraysForCustom->addItem(name);
    }
  ui->cobPMtypes->setCurrentIndex(ui->sbPMtype->value());  
  ui->cobPMtypeInArrays->setCurrentIndex(Detector->PMarrays[ui->cobUpperLowerPMs->currentIndex()].PMtype); //MainWindow::on_pbShowPMsArrayRegularData_clicked();
  ui->cobPMtypeInExplorers->setCurrentIndex( PMs->at(ui->sbIndPMnumber->value()).type );

  DoNotUpdateGeometry = tmpBool;
}

void MainWindow::on_cbLRFs_toggled(bool checked)
{
   if (checked) ui->twOption->setTabIcon(7, Rwindow->YellowIcon);
   else         ui->twOption->setTabIcon(7, QIcon());
}

void MainWindow::on_cbAreaSensitive_toggled(bool checked)
{
    if (checked) ui->twOption->setTabIcon(3, Rwindow->YellowIcon);
    else         ui->twOption->setTabIcon(3, QIcon());
}

void MainWindow::on_cbAngularSensitive_toggled(bool checked)
{
    RefreshAngularButtons();
    if (checked) ui->twOption->setTabIcon(2, Rwindow->YellowIcon);
    else         ui->twOption->setTabIcon(2, QIcon());
    ui->fAngular->setEnabled(checked);
    on_pbIndPMshowInfo_clicked(); //to refresh the binned button
}

void MainWindow::on_cbWaveResolved_toggled(bool checked)
{
    if (checked) ui->twOption->setTabIcon(1, Rwindow->YellowIcon);
    else         ui->twOption->setTabIcon(1, QIcon());

//    ui->fWaveTests->setEnabled(checked);
//    ui->fWaveOptions->setEnabled(checked);
//    ui->fPointSource_Wave->setEnabled(checked);
//    ui->fDirectOrmat->setEnabled(checked || ui->cbTimeResolved->isChecked());

    const int itype = ui->sbPMtype->value();
    const bool bHavePDE = (itype < PMs->countPMtypes() && !PMs->getType(itype)->PDE_lambda.isEmpty());
    ui->pbShowPDEbinned->setEnabled(checked && bHavePDE);
}

void MainWindow::on_cbTimeResolved_toggled(bool checked)
{
    ui->fTime->setEnabled(checked);
    if (checked) ui->twOption->setTabIcon(0, Rwindow->YellowIcon);
    else         ui->twOption->setTabIcon(0, QIcon());
//    ui->fPointSource_Time->setEnabled(checked);
//    ui->fDirectOrmat->setEnabled(ui->cbWaveResolved->isChecked() || ui->cbTimeResolved->isChecked());
}

void MainWindow::on_pbPMtypeShowCurrent_clicked()
{
  ui->tabWidget->setCurrentIndex(1);
  ui->sbPMtype->setValue(ui->sbPMtypeForGroup->value());
}

bool MainWindow::isWavelengthResolved() const
{
  return ui->cbWaveResolved->isChecked();
}

void MainWindow::recallGeometryOfLocalScriptWindow()
{
  if (!GenScriptWindow) return;

  GenScriptWindow->move(ScriptWinX, ScriptWinY);
  GenScriptWindow->resize(ScriptWinW, ScriptWinH);
}

void MainWindow::extractGeometryOfLocalScriptWindow()
{
  if (GenScriptWindow)
    {
      ScriptWinX = GenScriptWindow->x();
      ScriptWinY = GenScriptWindow->y();
      ScriptWinW = GenScriptWindow->width();
      ScriptWinH = GenScriptWindow->height();
    }
}

void MainWindow::on_ledWaveFrom_editingFinished()
{
  double newValue = ui->ledWaveFrom->text().toDouble();
  if (newValue<=0  || newValue > ui->ledWaveTo->text().toDouble())
    {
      QString str;
      str.setNum(WaveFrom,'g', 5);
      ui->ledWaveFrom->setText(str);      
      message("Error in the starting wavelength value, resetted", this);
      return;
    }
  MainWindow::CorrectWaveTo();
  //MainWindow::on_cbWaveResolved_toggled(ui->cbWaveResolved->isChecked());

  MainWindow::on_pbUpdateSimConfig_clicked();
}

void MainWindow::on_ledWaveTo_editingFinished()
{
  double newValue = ui->ledWaveTo->text().toDouble();
  if (newValue<=0  || newValue < ui->ledWaveFrom->text().toDouble())
    {
      QString str;
      str.setNum(WaveTo,'g', 5);
      ui->ledWaveTo->setText(str);      
      message("Error in the ending wavelength value, resetted", this);
      return;
    }
   MainWindow::CorrectWaveTo();
   //MainWindow::on_cbWaveResolved_toggled(ui->cbWaveResolved->isChecked());
   MainWindow::on_pbUpdateSimConfig_clicked();
}

void MainWindow::on_ledWaveStep_editingFinished()
{
   double newValue = ui->ledWaveStep->text().toDouble();
   if (newValue < 1e-5)
    {
      QString str;
      str.setNum(WaveStep,'g', 5);
      ui->ledWaveStep->setText(str);     
      message("Error in the step value, resetted", this);
      return;
    }
   MainWindow::CorrectWaveTo();
   //MainWindow::on_cbWaveResolved_toggled(ui->cbWaveResolved->isChecked());
   MainWindow::on_pbUpdateSimConfig_clicked();
}

void MainWindow::CorrectWaveTo()
{
  int steps = (ui->ledWaveTo->text().toDouble() - ui->ledWaveFrom->text().toDouble()) / ui->ledWaveStep->text().toDouble();
  WaveNodes = steps+1;
  QString str;
  str.setNum(WaveNodes);
  ui->labWaveOptionNodes->setText(str);
  double to = ui->ledWaveFrom->text().toDouble() + ui->ledWaveStep->text().toDouble()*steps;
  WaveTo = to;   
  str.setNum(to,'g',5);
  ui->ledWaveTo->setText(str);
  if (ui->sbFixedWaveIndexPointSource->value() > WaveNodes-1) ui->sbFixedWaveIndexPointSource->setValue(0);
}

void MainWindow::on_cobMaterialForWaveTests_currentIndexChanged(int index)
{
  if (index<0) return;
  if (DoNotUpdateGeometry) return;
  UpdateTestWavelengthProperties();
}

void MainWindow::on_pbUpdateTestWavelengthProperties_clicked()
{
  UpdateTestWavelengthProperties();
}

void MainWindow::UpdateTestWavelengthProperties()
{
  if (!isWavelengthResolved()) return;
  int matId = ui->cobMaterialForWaveTests->currentIndex();

  ui->pbTestShowPrimary->setEnabled((*MpCollection)[matId]->PrimarySpectrumHist);
  ui->pbTestShowSecondary->setEnabled((*MpCollection)[matId]->SecondarySpectrumHist);
  ui->pbTestGeneratorPrimary->setEnabled((*MpCollection)[matId]->PrimarySpectrumHist);
  ui->pbTestGeneratorSecondary->setEnabled((*MpCollection)[matId]->SecondarySpectrumHist);

  ui->pbTestShowRefrIndex->setEnabled((*MpCollection)[matId]->nWaveBinned.size());
  ui->pbTestShowAbs->setEnabled((*MpCollection)[matId]->absWaveBinned.size());
}


void MainWindow::on_pbTestShowPrimary_clicked()
{ 
  int matId = ui->cobMaterialForWaveTests->currentIndex();
  GraphWindow->Draw((*MpCollection)[matId]->PrimarySpectrumHist, "", true, false);
}

void MainWindow::on_pbTestShowSecondary_clicked()
{  
  int matId = ui->cobMaterialForWaveTests->currentIndex();
  GraphWindow->Draw((*MpCollection)[matId]->SecondarySpectrumHist, "", true, false);
}

void MainWindow::on_pbTestGeneratorPrimary_clicked()
{  
  int matId = ui->cobMaterialForWaveTests->currentIndex();

  if (!(*MpCollection)[matId]->PrimarySpectrumHist) return;

  auto hist1D = new TH1D("PrimaryTest", "Primary scintillation", WaveNodes, WaveFrom, WaveTo);

  for (int i=0; i<ui->sbWaveTestsNumPhotons->value(); i++)
      hist1D->Fill( (*MpCollection)[matId]->PrimarySpectrumHist->GetRandom() );
  GraphWindow->Draw(hist1D);
}

void MainWindow::on_pbTestGeneratorSecondary_clicked()
{  
  int matId = ui->cobMaterialForWaveTests->currentIndex();

  if (!(*MpCollection)[matId]->SecondarySpectrumHist) return;

  auto hist1D = new TH1D("SecondaryTest", "Secondary scintillation", WaveNodes, WaveFrom, WaveTo);

  for (int i=0; i<ui->sbWaveTestsNumPhotons->value(); i++)
      hist1D->Fill((*MpCollection)[matId]->SecondarySpectrumHist->GetRandom());
  GraphWindow->Draw(hist1D);
}

void MainWindow::on_pbTestShowRefrIndex_clicked()
{  
  int matId = ui->cobMaterialForWaveTests->currentIndex();

  QVector<double> x;
  x.resize(0);
  for (int i=0; i<WaveNodes; i++) x.append(WaveFrom + WaveStep*i);
  //ShowGraphWindow();
  GraphWindow->MakeGraph(&x, &(*MpCollection)[matId]->nWaveBinned, kRed, "Wavelength, nm", "Refractive index");
}

void MainWindow::on_pbTestShowAbs_clicked()
{  
  int matId = ui->cobMaterialForWaveTests->currentIndex();

  QVector<double> x;
  x.resize(0);
  for (int i=0; i<WaveNodes; i++) x.append(WaveFrom + WaveStep*i);
  //ShowGraphWindow();
  GraphWindow->MakeGraph(&x, &(*MpCollection)[matId]->absWaveBinned, kRed, "Wavelength, nm", "Attenuation coefficient, mm-1");
}

void MainWindow::on_sbFixedWaveIndexPointSource_valueChanged(int arg1)
{
  if (arg1 > WaveNodes-1)
    {
      ui->sbFixedWaveIndexPointSource->setValue(WaveNodes-1);
      return;
    }

  QString str;
  if (arg1 == -1)
    {
      str = "--";
    }
  else
    {
      double wave = WaveFrom + WaveStep * arg1;
      str.setNum(wave, 'g', 5);
      str += " nm";
    }
  ui->labWavelengthPointSource->setText(str);
}

void MainWindow::on_pbShowPDE_clicked()
{
   int typ = ui->sbPMtype->value();
   //ShowGraphWindow();
   if (ui->cobPMdeviceType->currentIndex() == 0)
     GraphWindow->MakeGraph(&PMs->getType(typ)->PDE_lambda, &PMs->getType(typ)->PDE, kRed, "Wavelength, nm", "Quantum Efficiency");
   else
     GraphWindow->MakeGraph(&PMs->getType(typ)->PDE_lambda, &PMs->getType(typ)->PDE, kRed, "Wavelength, nm", "Photon Detection Efficiency");
}

void MainWindow::on_pbLoadPDE_clicked()
{
  QString fileName;
  if (ui->cobPMdeviceType->currentIndex() == 0)
     fileName = QFileDialog::getOpenFileName(this, "Load quantum efficiency vs wavelength", GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*)");
  else
    fileName = QFileDialog::getOpenFileName(this, "Load photon detection efficiency vs wavelength", GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*)");
  //qDebug()<<fileName;

  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  QVector<double> x,y;
  LoadDoubleVectorsFromFile(fileName, &x, &y);

  for (int i=0; i<y.size(); i++)
    if (y[i]<0 || y[i]>1)
      {        
        message("Quantum Efficiency and Photon Detection Probability should be in the range from 0 to 1", this);
        return;
      }

  PMs->updateTypePDE(ui->sbPMtype->value(), &x, &y);
  //PMs->RebinPDEsForType(ui->sbPMtype->value());
  ReconstructDetector(true);

  MainWindow::on_pbRefreshPMproperties_clicked();
}

void MainWindow::on_pbDeletePDE_clicked()
{
  QVector<double> x,y;
  x.resize(0);
  y.resize(0);

  PMs->updateTypePDE(ui->sbPMtype->value(), &x, &y);
  //PMs->RebinPDEsForType(ui->sbPMtype->value());
  ReconstructDetector(true);

  MainWindow::on_pbRefreshPMproperties_clicked();
}

void MainWindow::on_pbScalePDE_clicked()
{
  int type = ui->sbPMtype->value();
  int numTypes = Detector->PMs->countPMtypes();
  if (type>numTypes-1) return;

  bool ok;
  double val = QInputDialog::getDouble(this, "PDE scaling",
                                             "Scale PDE data for PM type "+Detector->PMs->getType(type)->Name+"\n"
                                             "Scaling factor:", 1.0, -1e10, 1e10, 5, &ok);
  if (!ok) return;

  Detector->PMs->scaleTypePDE(type, val);
  //PMs->RebinPDEsForType(type);
  ReconstructDetector(true);

  MainWindow::on_pbRefreshPMproperties_clicked();
}

void MainWindow::on_pbShowPDEbinned_clicked()
{
  if (!ui->cbWaveResolved->isChecked())
    {      
      message("Activate wavelength resolved simulation option", this);
      return;
    }

  const int itype = ui->sbPMtype->value();
  //PMs->RebinPDEsForType(itype);

  QVector<double> x;
  for (int i = 0; i < WaveNodes; i++)
      x.append(WaveFrom + WaveStep * i);

  GraphWindow->MakeGraph(&x, &PMs->getType(itype)->PDEbinned, kRed, "Wavelength, nm", "PDE");
}

void MainWindow::on_pbPMtypeLoadAngular_clicked()
{
  QString fileName;
  fileName = QFileDialog::getOpenFileName(this, "Load angular response (0 - 90 degrees)", GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*)");
  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  QVector<double> x,y;
  LoadDoubleVectorsFromFile(fileName, &x, &y);
  if (x[0] !=0 || x[x.size()-1] != 90)
      {
        message("Data should start at 0 and end at 90 degrees!", this);
        return;
      }
  for (int i=0; i<x.size(); i++)
    if (y[i] < 0.0)
       {
         message("Data should be positive!", this);
         return;
       }
  if (y[0] <1.0e-10)
    {
      message("Response for normal incidence cannot be 0!", this);
      return;
    }
  double norm = 1.0/y[0];
  if (norm != 1.0)
    {
      for (int i=0; i<y.size(); i++) y[i] = y[i]*norm;
      message("Data were scaled to have response of unity at normal incidence", this);
    }
  int typ = ui->cobPMtypes->currentIndex();
  PMs->updateTypeAngular(typ, &x, &y);
  PMs->updateTypeAngularN1(typ, ui->ledMediumRefrIndex->text().toDouble());
  PMs->RecalculateAngularForType(typ);
  ReconstructDetector(true);
  MainWindow::RefreshAngularButtons();
}

void MainWindow::RefreshAngularButtons()
{
  ui->pbPMtypeShowAngular->setEnabled(PMs->getType(ui->sbPMtype->value())->AngularSensitivity_lambda.size());
  ui->pbPMtypeShowEffectiveAngular->setEnabled(ui->cbAngularSensitive->isChecked() && PMs->getType(ui->sbPMtype->value())->AngularSensitivity_lambda.size());
  ui->pbPMtypeDeleteAngular->setEnabled(PMs->getType(ui->sbPMtype->value())->AngularSensitivity_lambda.size());
}

void MainWindow::on_pbPMtypeShowAngular_clicked()
{  
  int typ = ui->cobPMtypes->currentIndex();
  GraphWindow->MakeGraph(&PMs->getType(typ)->AngularSensitivity_lambda, &PMs->getType(typ)->AngularSensitivity, kRed, "Angle of incidence, degrees", "Response");
}

void MainWindow::on_pbPMtypeDeleteAngular_clicked()
{
  int typ = ui->cobPMtypes->currentIndex();
  QVector<double> x;
  x.resize(0);
  PMs->updateTypeAngular(typ, &x, &x);
  PMs->RecalculateAngularForType(typ);
  MainWindow::RefreshAngularButtons();
  ReconstructDetector(true);
}

void MainWindow::on_pbPMtypeShowEffectiveAngular_clicked()
{  
  int itype = ui->cobPMtypes->currentIndex();

  PMs->RecalculateAngularForType(itype);

  QVector<double> x;
  int CosBins = ui->sbCosBins->value();
  for (int i=0; i<CosBins; i++) x.append(1.0/(CosBins-1)*i);  
  GraphWindow->MakeGraph(&x, &PMs->getType(itype)->AngularSensitivityCosRefracted, kRed, "Cosine of refracted beam", "Response");
}

void MainWindow::onGDMLstatusChage(bool fGDMLactivated)
{
    ui->fDisableTab1->setEnabled(!fGDMLactivated);
    ui->tabArray->setEnabled(!fGDMLactivated);
    ui->fPMtypeDisable1->setEnabled(!fGDMLactivated);
    ui->ledSizeX->setEnabled(!fGDMLactivated);
    ui->ledSizeY->setEnabled(!fGDMLactivated);
    ui->pbAddNewPMtype->setEnabled(!fGDMLactivated);
    ui->pbRemoveThisPMtype->setEnabled(!fGDMLactivated);
    ui->fDisableIndividual->setEnabled(!fGDMLactivated);
    ui->pbIndPmRemove->setEnabled(!fGDMLactivated);
    ui->pbRemoveMaterial->setEnabled(!fGDMLactivated); //cannot remove materials if GDML is loaded

    ui->pbGDML->setVisible(fGDMLactivated);   
}

void MainWindow::on_ledMediumRefrIndex_editingFinished()
{
    PMs->updateTypeAngularN1(ui->sbPMtype->value(), ui->ledMediumRefrIndex->text().toDouble());
    PMs->RecalculateAngularForType(ui->sbPMtype->value());
    ReconstructDetector(true);
}

void MainWindow::on_pbPMtypeLoadArea_clicked()
{
  QString fileName;
  fileName = QFileDialog::getOpenFileName(this, "Load area response", GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*)");
  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  QVector< QVector <double> > tmp;
  double xStep, yStep;
  int error = MainWindow::LoadAreaResponse(fileName, &tmp, &xStep, &yStep);

  if (error == 0)
    {
      PMs->updateTypeArea(ui->sbPMtype->value(), &tmp, xStep, yStep);
      MainWindow::RefreshAreaButtons();
    }
  ReconstructDetector(true);
}

void MainWindow::RefreshAreaButtons()
{
  ui->pbPMtypeShowArea->setEnabled(PMs->getType(ui->sbPMtype->value())->AreaSensitivity.size());
  ui->pbPMtypeDeleteArea->setEnabled(PMs->getType(ui->sbPMtype->value())->AreaSensitivity.size());
}

void MainWindow::on_pbPMtypeShowArea_clicked()
{
  int typ = ui->sbPMtype->value();

  //PMpropertiesStruct tp = *PMs->getTypeProperties(typ);

  if (PMs->getType(typ)->AreaSensitivity.isEmpty()) return;
  int xNum = PMs->getType(typ)->AreaSensitivity.size();
  int yNum = PMs->getType(typ)->AreaSensitivity[0].size();
  double xStep=PMs->getType(typ)->AreaStepX;
  double yStep=PMs->getType(typ)->AreaStepY;

//  qDebug()<<xNum<<xStep<<yNum<<yStep;

  auto hist2D = new TH2D("T-AreaSens","Response",xNum, -0.5*xNum*xStep, +0.5*xNum*xStep, yNum, -0.5*yNum*yStep, +0.5*yNum*yStep);

  for (int iX=0; iX<xNum; iX++)
   for (int iY=0; iY<yNum; iY++)
       hist2D->SetCellContent(iX+1, iY+1, PMs->getType(typ)->AreaSensitivity[iX][iY]);

  hist2D->SetXTitle("X, mm");
  hist2D->SetYTitle("Y, mm");
  GraphWindow->Draw(hist2D, "colz");
}

void MainWindow::on_pbPMtypeDeleteArea_clicked()
{
   PMs->clearTypeArea(ui->sbPMtype->value());
   MainWindow::RefreshAreaButtons();
   ReconstructDetector(true);
}

void MainWindow::on_cobPMarrayRegularity_currentIndexChanged(int index)
{
  int ul = ui->cobUpperLowerPMs->currentIndex();
  Detector->PMarrays[ul].Regularity = index;

  if (index == 0) ui->swPMarray->setCurrentIndex(0);
  else ui->swPMarray->setCurrentIndex(1);

  if (index == 1)
    {
      ui->pbLoadPMcenters->setText("Load PM positions (X,Y)");
      ui->pbSavePMcenters->setText("Save center positions (X,Y)");
    }
  else if (index == 2)
    {
      ui->pbLoadPMcenters->setText("Load PM positions (X,Y,Z)");
      ui->pbSavePMcenters->setText("Save center positions (X,Y,Z)");
    }

  if (index < 2)
    {
      //on changed from irregular to any regular - has to rebuild the detector
      MainWindow::on_pbRefreshPMArrayData_clicked();
    }
}

int MainWindow::PMArrayType(int ul)
{
  //return ui->cobPMarrayRegularity->currentIndex();
  return Detector->PMarrays[ul].Regularity;
}

void MainWindow::SetPMarrayType(int ul, int itype)
{
  //ui->cobPMarrayRegularity->setCurrentIndex(itype);
  Detector->PMarrays[ul].Regularity = itype;
}

void MainWindow::on_sbIndPMnumber_valueChanged(int arg1)
{
  if (arg1 == 0   &&   PMs->count() ==0) return;
  if (arg1 > PMs->count()-1)
    {
      if (PMs->count() == 0)
        {
          ui->fIndPM->setEnabled(false);
          ui->sbIndPMnumber->setValue(0);
          return;
        }
      ui->sbIndPMnumber->setValue(PMs->count()-1);
      return;
    }

  ui->fIndPM->setEnabled(true);
  MainWindow::on_pbIndPMshowInfo_clicked();
}

void MainWindow::on_pbIndPMshowInfo_clicked()
{
    //qDebug() << "--PM explorer indication--";
    if (PMs->count() == 0) return;
    int ipm = ui->sbIndPMnumber->value();
    int ul, index;
    Detector->findPM(ipm, ul, index);
    if (index<0) return;
    APmPosAngTypeRecord *p = &Detector->PMarrays[ul].PositionsAnglesTypes[index];
    APm *PM = &PMs->at(ipm);

    //format should be the same in MainWindow::on_pbUpdateToFixedZ_clicked() and MainWindow::on_pbUpdateToFullCustom_clicked()
    ui->ledIndPMx->setText(QString::number(PM->x, 'g', 4));
    ui->ledIndPMy->setText(QString::number(PM->y, 'g', 4));
    ui->ledIndPMz->setText(QString::number(PM->z, 'g', 4));

    ui->ledIndPMphi->setText(QString::number(PM->phi, 'g', 4));
    ui->ledIndPMtheta->setText(QString::number(PM->theta, 'g', 4));
    ui->ledIndPMpsi->setText(QString::number(PM->psi, 'g', 4));
    ui->cobPMtypeInExplorers->setCurrentIndex(PM->type);

    //-- overrides --
    //effective DE    
    double eDE = PMs->at(ipm).effectivePDE;
    QString str;
    APmType* type = PMs->getType(p->type);
    if (eDE == -1)
      {
        str.setNum(type->EffectivePDE);
        ui->labIndDEStatus->setText("Inherited:");
        //ui->pbIndRestoreEffectiveDE->setEnabled(false);
      }
    else
      {
        str.setNum(eDE, 'g', 4);
        ui->labIndDEStatus->setText("<b>Override:</b>");
        //ui->pbIndRestoreEffectiveDE->setEnabled(true);
      }
    ui->ledIndEffectiveDE->setText(str);
    //wave-resolved DE
    if (PMs->isPDEwaveOverriden(ipm))
      {
        ui->labIndDEwaveStatus->setText("<b>Override:</b>");
        ui->pbIndRestoreDE->setEnabled(true);
        ui->pbIndShowDE->setEnabled(true);
        if (ui->cbWaveResolved->isChecked()) ui->pbIndShowDEbinned->setEnabled(true);
        else ui->pbIndShowDEbinned->setEnabled(false);
      }
    else
      {       
       ui->pbIndRestoreDE->setEnabled(false);
       if (type->PDE.size()>0)
         {
          ui->labIndDEwaveStatus->setText("Inherited:");
          ui->pbIndShowDE->setEnabled(true);          
          if (ui->cbWaveResolved->isChecked()) ui->pbIndShowDEbinned->setEnabled(true);
          else ui->pbIndShowDEbinned->setEnabled(false);
         }
       else
         {
          ui->labIndDEwaveStatus->setText("Not defined");
          ui->pbIndShowDE->setEnabled(false);
          ui->pbIndShowDEbinned->setEnabled(false);
         }
    }
    //Angular
    if (PMs->isAngularOverriden(ipm))
      {
        ui->labIndAngularStatus->setText("<b>Override:</b>");
        ui->pbIndRestoreAngular->setEnabled(true);
        ui->pbIndShowAngular->setEnabled(true);
        if (ui->cbAngularSensitive->isChecked()) ui->pbIndShowEffectiveAngular->setEnabled(true);
        else ui->pbIndShowEffectiveAngular->setEnabled(false);
      }
    else
      {
        ui->pbIndRestoreAngular->setEnabled(false);
        if (type->AngularSensitivity.size()>0)
          {
           ui->labIndAngularStatus->setText("Inherited:");
           ui->pbIndShowAngular->setEnabled(true);
           if (ui->cbAngularSensitive->isChecked()) ui->pbIndShowEffectiveAngular->setEnabled(true);
           else ui->pbIndShowEffectiveAngular->setEnabled(false);
          }
        else
          {
           ui->labIndAngularStatus->setText("Not defined");
           ui->pbIndShowAngular->setEnabled(false);
           ui->pbIndShowEffectiveAngular->setEnabled(false);
          }
      }
    //Area
    if (PMs->isAreaOverriden(ipm))
      {
         ui->labIndAreaStatus->setText("<b>Override:</b>");
         ui->pbIndRestoreArea->setEnabled(true);
         ui->pbIndShowArea->setEnabled(true);
      }
    else
      {         
         ui->pbIndRestoreArea->setEnabled(false);
         if (type->AreaSensitivity.size()>0)
           {
             ui->labIndAreaStatus->setText("Inherited:");
             ui->pbIndShowArea->setEnabled(true);
           }
         else
           {
             ui->labIndAreaStatus->setText("Not defined");
             ui->pbIndShowArea->setEnabled(false);
           }
      }

    //show this PM
    if (ui->tabWidget->currentIndex()==3 && ui->tabwidMain->currentIndex()==0)
      {        
        Detector->GeoManager->ClearTracks();      
        double length = type->SizeX*0.5;
        Int_t track_index = Detector->GeoManager->AddTrack(1,22);
        TVirtualGeoTrack *track = Detector->GeoManager->GetTrack(track_index);      
        track->AddPoint(PM->x-length, PM->y, PM->z, 0);
        track->AddPoint(PM->x+length, PM->y, PM->z, 0);
        track->AddPoint(PM->x, PM->y, PM->z, 0);
        track->AddPoint(PM->x, PM->y+length, PM->z, 0);
        track->AddPoint(PM->x, PM->y-length, PM->z, 0);
        track->AddPoint(PM->x, PM->y, PM->z, 0);
        track->AddPoint(PM->x, PM->y, PM->z+length, 0);
        track->AddPoint(PM->x, PM->y, PM->z-length, 0);
        track->SetLineWidth(4);
        track->SetLineColor(kBlack);

        //show orientation
        double l[3] = {0,0,length}; //loacl coordinates (with PM)
        double m[3]; //master coordinates (world)
        TGeoRotation Rot = TGeoRotation("Rot", PM->phi, PM->theta, PM->psi);
        Rot.LocalToMaster(l, m);
        track_index = Detector->GeoManager->AddTrack(1,22);
        track = Detector->GeoManager->GetTrack(track_index);
        track->AddPoint(PM->x, PM->y, PM->z, 0);
        track->AddPoint(PM->x+m[0], PM->y+m[1], PM->z+m[2], 0);
        track->SetLineWidth(4);
        track->SetLineColor(kRed);

        GeometryWindow->DrawTracks();
      }
}

void MainWindow::on_tabWidget_currentChanged(int index)
{  
   if (index == 3)
     {
       if (PMs->count() == 0) ui->fIndPM->setEnabled(false);
       else ui->fIndPM->setEnabled(true);
       MainWindow::on_sbIndPMnumber_valueChanged(ui->sbIndPMnumber->value());
     }  
}

void MainWindow::on_pbUpdateToFullCustom_clicked()
{
  if (TriggerForbidden) return;
  int ipm = ui->sbIndPMnumber->value();
  int ul, index;
  Detector->findPM(ipm, ul, index);
  if (ipm<0) return; //protection: something is wrong

  APmPosAngTypeRecord *p = &Detector->PMarrays[ul].PositionsAnglesTypes[index];

  //protection agains click_in-click_out trigger causing warning about leaving regular array
  //and we want to avoid problems due to rounding of doubles
  bool flagMod=false;
  QString gui, data;
  gui = ui->ledIndPMz->text();
  gui.setNum(gui.toDouble(), 'g', 4); //logically the same e.g. 15 and 15.0
  data.setNum(p->z, 'g', 4); // format should match format used in MainWindow::on_pbIndPMshowInfo_clicked()
  if (gui != data) flagMod = true;
  gui = ui->ledIndPMtheta->text();
  gui.setNum(gui.toDouble(), 'g', 4); //logically the same e.g. 15 and 15.0
  data.setNum(p->theta, 'g', 4); // format should match format used in MainWindow::on_pbIndPMshowInfo_clicked()
  if (gui != data) flagMod = true;
  gui = ui->ledIndPMphi->text();
  gui.setNum(gui.toDouble(), 'g', 4); //logically the same e.g. 15 and 15.0
  data.setNum(p->phi, 'g', 4); // format should match format used in MainWindow::on_pbIndPMshowInfo_clicked()
  if (gui != data) flagMod = true;

  if (!flagMod) return; //nothing was actually modified
  if (Detector->PMarrays[ul].Regularity < 2)
    {
      TriggerForbidden = true;
      int ret = QMessageBox::question(this, "Array type change confirmation",
                                      "This modification will require to change PM Array mode\n"
                                      "to 'Fully custom'.\n"
                                      "Do you confirm the change?",
                                      QMessageBox::Yes | QMessageBox::No);
      TriggerForbidden = false;
      if (ret == QMessageBox::No)
        {
          MainWindow::on_pbIndPMshowInfo_clicked();
          return;
        }
    }
  //modification was autorized
  p->z = ui->ledIndPMz->text().toDouble();
  p->theta = ui->ledIndPMtheta->text().toDouble();
  p->phi = ui->ledIndPMphi->text().toDouble();
  if (Detector->PMarrays[ul].Regularity != 2) Detector->PMarrays[ul].Regularity = 2;

  MainWindow::ReconstructDetector();
  MainWindow::on_pbIndPMshowInfo_clicked();
  MainWindow::on_pbShowPMsArrayRegularData_clicked();
}

void MainWindow::on_pbUpdateToFixedZ_clicked()
{
  if (TriggerForbidden) return;
  int ipm = ui->sbIndPMnumber->value();
  int ul, index;
  Detector->findPM(ipm, ul, index);
  if (ipm<0) return; //protection: something is wrong

  APmPosAngTypeRecord *p = &Detector->PMarrays[ul].PositionsAnglesTypes[index];

  //protection agains click_in-click_out trigger causing warning about leaving regular array
  //and we want to avoid problems due to rounding of doubles
  bool flagMod=false;
  QString gui, data;
  gui = ui->ledIndPMx->text();
  gui.setNum(gui.toDouble(), 'g', 4); //logically the same e.g. 15 and 15.0
  data.setNum(p->x, 'g', 4); // format should match format used in MainWindow::on_pbIndPMshowInfo_clicked()
  if (gui != data) flagMod = true;
  gui = ui->ledIndPMy->text();
  gui.setNum(gui.toDouble(), 'g', 4);
  data.setNum(p->y, 'g', 4);
  if (gui != data) flagMod = true;
  gui = ui->ledIndPMpsi->text();
  gui.setNum(gui.toDouble(), 'g', 4);
  data.setNum(p->psi, 'g', 4);
  if (gui != data) flagMod = true;
  if (ui->cobPMtypeInExplorers->currentIndex() != p->type) flagMod = true;

  if (!flagMod) return; //nothing was actually modified
  if (Detector->PMarrays[ul].Regularity == 0)
    {
      TriggerForbidden = true;
      int ret = QMessageBox::question(this, "Array type change confirmation",
                                      "This modification will require to change PM Array mode\n"
                                      "to 'Custom with auto-Z'.\n"
                                      "Do you confirm the change?",
                                      QMessageBox::Yes | QMessageBox::No);
      TriggerForbidden = false;
      if (ret == QMessageBox::No)
        {
          MainWindow::on_pbIndPMshowInfo_clicked();
          return;
        }
    }
  //modification was autorized
  p->x = ui->ledIndPMx->text().toDouble();
  p->y = ui->ledIndPMy->text().toDouble();
  p->psi = ui->ledIndPMpsi->text().toDouble();  
  p->type = ui->cobPMtypeInExplorers->currentIndex();
  if (Detector->PMarrays[ul].Regularity == 0) Detector->PMarrays[ul].Regularity = 1; //if it is 1 or 2 - leave as is

  MainWindow::ReconstructDetector(true);
  MainWindow::on_pbIndPMshowInfo_clicked();
  MainWindow::on_pbShowPMsArrayRegularData_clicked();
}

void MainWindow::on_pbIndPmRemove_clicked()
{
  QMessageBox msgBox(this);
  msgBox.setWindowTitle("Confirmation");
  msgBox.setText("Remove this PM?");
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox.setDefaultButton(QMessageBox::Yes);
  if (msgBox.exec() == QMessageBox::No) return;

  int ipm = ui->sbIndPMnumber->value();
  int ul, index;
  Detector->findPM(ipm, ul, index);
  if (index < 0) return;
  if (Detector->PMarrays[ul].Regularity == 0) Detector->PMarrays[ul].Regularity = 1;
  Detector->PMarrays[ul].PositionsAnglesTypes.remove(index);

  MainWindow::ReconstructDetector();

  ipm--;
  if (ipm<0) ipm =0;
  ui->sbIndPMnumber->setValue(ipm);
  MainWindow::ShowPMcount();
  MainWindow::on_pbIndPMshowInfo_clicked();
  MainWindow::on_pbShowPMsArrayRegularData_clicked();
  ReconstructDetector(false);
}

void MainWindow::on_pbIndShowType_clicked()
{
  ui->tabWidget->setCurrentIndex(1);
  ui->sbPMtype->setValue(ui->cobPMtypeInExplorers->currentIndex());
}

void MainWindow::on_ledIndEffectiveDE_editingFinished()
{
    const int ipm = ui->sbIndPMnumber->value();
    const double val = ui->ledIndEffectiveDE->text().toDouble();

    PMs->at(ipm).effectivePDE = val;

    ReconstructDetector(true);
    //MainWindow::on_pbIndPMshowInfo_clicked();
}

void MainWindow::on_pbIndRestoreEffectiveDE_clicked()
{
    const int ipm = ui->sbIndPMnumber->value();
    PMs->at(ipm).effectivePDE = -1.0;
    ReconstructDetector(true);
}

void MainWindow::on_pbIndShowDE_clicked()
{  
  int ipm = ui->sbIndPMnumber->value();
  int typ = ui->cobPMtypeInExplorers->currentIndex();

  const TString tit = ( PMs->isSiPM(ipm) ? "Photon detection efficiency" : "Quantum efficiency" );
  if (PMs->isPDEwaveOverriden(ipm))
      GraphWindow->MakeGraph(&PMs->at(ipm).PDE_lambda, &PMs->at(ipm).PDE, kRed, "Wavelength, nm", tit);
  else
      GraphWindow->MakeGraph(&PMs->getType(typ)->PDE_lambda, &PMs->getType(typ)->PDE, kRed, "Wavelength, nm", tit);
}

void MainWindow::on_pbIndRestoreDE_clicked()
{
    const int ipm = ui->sbIndPMnumber->value();

    PMs->at(ipm).PDE.clear();
    PMs->at(ipm).PDE_lambda.clear();
    PMs->at(ipm).PDEbinned.clear();

    ReconstructDetector(true);
}

void MainWindow::on_pbIndLoadDE_clicked()
{
  int ipm = ui->sbIndPMnumber->value();

  QString fileName;
  if (PMs->isSiPM(ipm))
     fileName = QFileDialog::getOpenFileName(this, "Load photon detection efficiency vs wavelength", GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*)");
  else
     fileName = QFileDialog::getOpenFileName(this, "Load quantum efficiency vs wavelength", GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*)");

  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  QVector<double> x,y;
  LoadDoubleVectorsFromFile(fileName, &x, &y);

  for (int i=0; i<y.size(); i++)
    if (y[i]<0 || y[i]>1)
      {
        message("Quantum Efficiency and Photon Detection Probability should be in the range from 0 to 1", this);
        return;
      }

  PMs->setPDEwave(ipm, &x, &y);
  PMs->RebinPDEsForPM(ipm);
  ReconstructDetector(true);

  MainWindow::on_pbIndPMshowInfo_clicked();
}

void MainWindow::on_pbIndShowDEbinned_clicked()
{
  if (!ui->cbWaveResolved->isChecked())
    {      
      message("First activate wavelength resolved simulation option", this);
      return;
    }

  int ipm = ui->sbIndPMnumber->value();
  int typ = ui->cobPMtypeInExplorers->currentIndex();

  QVector<double> x;
  for (int i=0; i<WaveNodes; i++) x.append(WaveFrom + WaveStep*i);

  if (PMs->at(ipm).PDEbinned.size() > 0)
    GraphWindow->MakeGraph(&x, &PMs->at(ipm).PDEbinned, kRed, "Wavelength, nm", "PDE");
  else
    GraphWindow->MakeGraph(&x, &PMs->getType(typ)->PDEbinned, kRed, "Wavelength, nm", "PDE");
}

void MainWindow::on_pbAddPM_clicked()
{
  int ul = ui->cobUpperLowerPMs->currentIndex();
  int ityp = ui->sbPMtype->value();
  Detector->PMarrays[ul].PositionsAnglesTypes.append(APmPosAngTypeRecord(0,0,0, 0,0,0, ityp));
  MainWindow::ReconstructDetector();
  int ipm = (ul == 0) ?  Detector->PMarrays[0].PositionsAnglesTypes.size()-1 :
                         Detector->PMarrays[0].PositionsAnglesTypes.size() + Detector->PMarrays[1].PositionsAnglesTypes.size()-1;
  ui->sbIndPMnumber->setValue(ipm);
  ui->tabWidget->setCurrentIndex(3);
}

void MainWindow::on_pbIndLoadAngular_clicked()
{
  QString fileName;
  fileName = QFileDialog::getOpenFileName(this, "Load angular response (0 - 90 degrees)", GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*)");
  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  QVector<double> x,y;
  LoadDoubleVectorsFromFile(fileName, &x, &y);

  if (x[0] !=0 || x[x.size()-1] != 90)
      {
        message("Data should start at 0 and end at 90 degrees!", this);
        return;
      }

  for (int i=0; i<x.size(); i++)
      if (y[i] < 0)
       {
         message("Data should be positive!", this);
         return;
       }

  if (y[0] <1e-10)
    {
      message("Response for normal incidence cannot be 0 (due to the auto-scale it to 1)!", this);
      return;
    }

  double norm = y[0];
  if (norm != 1)
    {
      for (int i=0; i<x.size(); i++) y[i] = y[i]/norm;
      message("Data were scaled to have response of unity at normal incidence", this);
    }

  int ipm = ui->sbIndPMnumber->value();
  PMs->setAngular(ipm, &x, &y);
  PMs->at(ipm).AngularN1 = ui->ledIndMediumRefrIndex->text().toDouble();

  PMs->RecalculateAngularForPM(ipm);
  ReconstructDetector(true);
  //MainWindow::on_pbIndPMshowInfo_clicked();
}

void MainWindow::on_pbIndRestoreAngular_clicked()
{
  const int ipm = ui->sbIndPMnumber->value();

  PMs->at(ipm).AngularSensitivity_lambda.clear();
  PMs->at(ipm).AngularSensitivity.clear();
  PMs->at(ipm).AngularSensitivityCosRefracted.clear();

  ReconstructDetector(true);
  //MainWindow::on_pbIndPMshowInfo_clicked();
}

void MainWindow::on_pbIndShowAngular_clicked()
{  
  const int ipm = ui->sbIndPMnumber->value();
  const int itype = ui->cobPMtypeInExplorers->currentIndex();

  if (PMs->isAngularOverriden(ipm))
        GraphWindow->MakeGraph(&PMs->at(ipm).AngularSensitivity_lambda, &PMs->at(ipm).AngularSensitivity, kRed, "Angle, degrees", "Response");
  else
        GraphWindow->MakeGraph(&PMs->getType(itype)->AngularSensitivity_lambda, &PMs->getType(itype)->AngularSensitivity, kRed, "Angle, degrees", "Response");
}

void MainWindow::on_pbIndShowEffectiveAngular_clicked()
{
  if (!ui->cbAngularSensitive->isChecked())
    {
      message("First activate angular resolved simulation option", this);
      return;
    }

  int ipm = ui->sbIndPMnumber->value();
  int typ = ui->cobPMtypeInExplorers->currentIndex();

  QVector<double> x;
  int CosBins = ui->sbCosBins->value();
  for (int i=0; i<CosBins; i++) x.append(1.0/(CosBins-1)*i);

  if (PMs->at(ipm).AngularSensitivityCosRefracted.size() > 0)
    GraphWindow->MakeGraph(&x, &PMs->at(ipm).AngularSensitivityCosRefracted, kRed, "Cosine of refracted beam", "Response");
  else
    GraphWindow->MakeGraph(&x, &PMs->getType(typ)->AngularSensitivityCosRefracted, kRed, "Cosine of refracted beam", "Response");
}

void MainWindow::on_ledIndMediumRefrIndex_editingFinished()
{
    const int ipm = ui->sbIndPMnumber->value();

    PMs->at(ipm).AngularN1 = ui->ledIndMediumRefrIndex->text().toDouble();

    PMs->RecalculateAngularForPM(ipm);
    ReconstructDetector(true);
    //MainWindow::on_pbIndPMshowInfo_clicked();
}

void MainWindow::on_pbIndLoadArea_clicked()
{
    QString fileName;
    fileName = QFileDialog::getOpenFileName(this, "Load area response", GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*)");
    if (fileName.isEmpty()) return;
    GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

    int ipm = ui->sbIndPMnumber->value();
    QVector< QVector <double> > tmp;
    double xStep, yStep;
    int error = MainWindow::LoadAreaResponse(fileName, &tmp, &xStep, &yStep);

    if (error == 0) PMs->setArea(ipm, &tmp, xStep, yStep);
    //if (error == 1) message("Error reading the file - cannot open!", this);
    //if (error == 2) message("Error reading the file - wrong format!", this);
    ReconstructDetector(true);

    MainWindow::on_pbIndPMshowInfo_clicked();
}

void MainWindow::on_pbIndRestoreArea_clicked()
{
    int ipm = ui->sbIndPMnumber->value();
    QVector<QVector<double> > tmp;
    PMs->setArea(ipm, &tmp, 123.0, 123.0); //strange step deliberately
    ReconstructDetector(true);
}

void MainWindow::on_pbIndShowArea_clicked()
{
    int ipm = ui->sbIndPMnumber->value();
    int typ = ui->sbPMtype->value();

    int xNum;
    int yNum;
    double xStep;
    double yStep;

    if (PMs->isAreaOverriden(ipm))
      {
        xNum = PMs->at(ipm).AreaSensitivity.size();
        yNum = PMs->at(ipm).AreaSensitivity.at(0).size();
        xStep = PMs->at(ipm).AreaStepX;
        yStep = PMs->at(ipm).AreaStepY;
      }
    else
      {
        if (PMs->getType(typ)->AreaSensitivity.isEmpty()) return; //then nothing to show
        xNum = PMs->getType(typ)->AreaSensitivity.size();
        yNum = PMs->getType(typ)->AreaSensitivity.at(0).size();
        xStep=PMs->getType(typ)->AreaStepX;
        yStep=PMs->getType(typ)->AreaStepY;
      }
//    qDebug()<<xNum<<xStep<<yNum<<yStep;

    auto hist2D = new TH2D("T-AreaSens","Response, %",xNum, -0.5*xNum*xStep, +0.5*xNum*xStep, yNum, -0.5*yNum*yStep, +0.5*yNum*yStep);

    for (int iX=0; iX<xNum; iX++)
     for (int iY=0; iY<yNum; iY++)
     {
        if (PMs->isAreaOverriden(ipm))
            hist2D->SetCellContent(iX+1, iY+1, PMs->at(ipm).AreaSensitivity.at(iX).at(iY) * 100.0);
        else
            hist2D->SetCellContent(iX+1, iY+1, PMs->getType(typ)->AreaSensitivity.at(iX).at(iY) * 100.0);
     }
    hist2D->SetXTitle("X, mm");
    hist2D->SetYTitle("Y, mm");
    GraphWindow->Draw(hist2D, "colz");
}

void MainWindow::on_cbGunAllowMultipleEvents_toggled(bool checked)
{
    ui->fGunMultipleEvents->setVisible(checked);
}

void MainWindow::UpdatePreprocessingSettingsIndication()
{
  APreprocessingSettings set;
  set.readFromJson(Config->JSON, Detector->PMs, "");

  ui->cbPMsignalPreProcessing->setChecked(set.fActive);

  on_sbPreprocessigPMnumber_valueChanged(ui->sbPreprocessigPMnumber->value());
  //if (ui->sbPreprocessigPMnumber->value() == 0) on_sbPreprocessigPMnumber_valueChanged(0);
  //else ui->sbPreprocessigPMnumber->setValue(0);

  if (set.fActive)
    {
      ui->cbLoadedDataHasEnergy->setChecked(set.fHaveLoadedEnergy);
      if (set.fHaveLoadedEnergy) ui->sbLoadedEnergyChannelNumber->setValue(set.EnergyChannel);

      ui->cbLoadedDataHasPosition->setChecked(set.fHavePosition);
      if (set.fHavePosition) ui->sbLoadASCIIpositionXchannel->setValue(set.PositionXChannel);

      ui->cbLoadedDataHasZPosition->setChecked(set.fHaveZPosition);
      if (set.fHaveZPosition) ui->sbLoadASCIIpositionZchannel->setValue(set.PositionZChannel);

      ui->cbIgnoreEvent->setChecked(set.fIgnoreThresholds);
       if (set.fIgnoreThresholds)
         {
           ui->ledIgnoreThresholdMin->setText( QString::number(set.ThresholdMin) );
           ui->ledIgnoreThresholdMax->setText( QString::number(set.ThresholdMax) );
         }

      ui->cbLimitNumberEvents->setChecked(set.fLimitNumber);
      if (set.fLimitNumber) ui->sbFirstNevents->setValue(set.LimitMax);

      if (set.fManifest) ui->leManifestFile->setText(set.ManifestFile);
      else ui->leManifestFile->setText("");


    }

  TmpHub->PreEnAdd = set.LoadEnAdd;
  TmpHub->PreEnMulti = set.LoadEnMulti;
}

void MainWindow::LoadPMsignalsRequested()
{
  QStringList fileNames = QFileDialog::getOpenFileNames(this, "Load events from ascii file(s)", GlobSet.LastOpenDir, "Data files (*.dat *.txt);;All files (*)");
  if (fileNames.isEmpty()) return;
  this->activateWindow();
  this->raise();
  GlobSet.LastOpenDir = QFileInfo(fileNames.first()).absolutePath();

  if (LoadedEventFiles.isEmpty()) MainWindow::ClearData(); // first load - make sure there is no data (e.g. sim events)
  else
    {
#ifdef ANTS_FLANN
   ReconstructionManager->KNNmodule->Filter.clear();
#endif
    }

  fStopLoadRequested = false;  
  ui->fLoadProgress->setVisible(true);
  ui->pbStopLoad->setVisible(true);
  QString SnumFiles = QString::number(fileNames.size());
  ui->labFilesProgress->setText("0 / "+SnumFiles);
  for (int i=0; i<fileNames.size(); i++)
    {
      qApp->processEvents();

      if (fStopLoadRequested) break;

      ui->labFilesProgress->setText(QString::number(i+1)+" / "+SnumFiles);
      int loadedEvents = MainWindow::LoadPMsignals(fileNames[i]);           
      if (loadedEvents>0)
        {
          //qDebug()<<"Loaded"<<loadedEvents<<"events from file"<<fileNames[i];
          ui->leoLoadedEvents->setText(QString::number(loadedEvents));
          ui->leoTotalLoadedEvents->setText(QString::number(EventsDataHub->Events.size()));
          LoadedEventFiles.append(fileNames[i]);
          QListWidgetItem* item = new QListWidgetItem(fileNames[i]);
          item->setFlags (item->flags () | Qt::ItemIsEditable);
          ui->lwLoadedEventsFiles->addItem(item);
        }
      else ui->leoLoadedEvents->setText("0");
    }
  ui->fLoadProgress->setVisible(false);
  ui->pbStopLoad->setVisible(false);
  if (!EventsDataHub->Events.isEmpty())
    {
      ui->cbPMsignalPreProcessing->setEnabled(true);      
      //ui->pbReloadExpData->setEnabled(true);
      Owindow->RefreshData();
      Rwindow->OnEventsDataAdded();
      EventsDataHub->squeeze();
    }
  //ui->leoTotalLoadedEvents->setText(QString::number(EventsDataHub->Events.size()));  
}

void MainWindow::on_pbReloadExpData_clicked()
{
  ui->fReloadRequired->setVisible(false);
  if (LoadedEventFiles.isEmpty())
    {
      message("File list is empty!", this);
      MainWindow::DeleteLoadedEvents();
      return;
    }
  MainWindow::DeleteLoadedEvents(true); //keeping the FileList intact!

  fStopLoadRequested = false;
  WindowNavigator->BusyOn();
  ui->fLoadProgress->setVisible(true);
  ui->pbStopLoad->setVisible(true);
  QString SnumFiles = QString::number(LoadedEventFiles.size());
  ui->labFilesProgress->setText("0 / "+SnumFiles);
  for (int i=0; i<LoadedEventFiles.size(); i++)
    {
      qApp->processEvents();

      if (fStopLoadRequested)
      {
          qApp->processEvents();
          ui->fReloadRequired->setVisible(true);
          break;
      }

      ui->labFilesProgress->setText(QString::number(i+1)+" / "+SnumFiles);
      int loadedEvents = MainWindow::LoadPMsignals(LoadedEventFiles[i]);
      //if (loadedEvents>0) qDebug()<<"Loaded"<<loadedEvents<<"events from file"<<LoadedEventFiles[i];
      if (loadedEvents<0) break;
      ui->leoLoadedEvents->setText(QString::number(loadedEvents));
      ui->leoTotalLoadedEvents->setText(QString::number(EventsDataHub->Events.size()));
    }
  ui->fLoadProgress->setVisible(false);
  ui->pbStopLoad->setVisible(false);
  WindowNavigator->BusyOff();

  if (!EventsDataHub->Events.isEmpty())
    {      
      Owindow->RefreshData();
      Rwindow->OnEventsDataAdded();
    }

  ui->leoTotalLoadedEvents->setText(QString::number(EventsDataHub->Events.size()));

  EventsDataHub->squeeze();
}

void MainWindow::LoadEventsListContextMenu(const QPoint &pos)
{
    QMenu myMenu;
    myMenu.addSeparator();
    myMenu.addAction("Clone");
    myMenu.addSeparator();
    myMenu.addAction("Remove");
    myMenu.addSeparator();

    QPoint globalPos = ui->lwLoadedEventsFiles->mapToGlobal(pos);
    QAction* selectedItem = myMenu.exec(globalPos);

    if (selectedItem)
    {
        int row = ui->lwLoadedEventsFiles->currentRow();

        if (selectedItem->iconText() == "Clone")
         {
            //clone the line
            if (row>-1)
              {
                QListWidgetItem* newitem = ui->lwLoadedEventsFiles->item(row)->clone();
                ui->lwLoadedEventsFiles->insertItem(row, newitem);
                LoadedEventFiles.insert(row, LoadedEventFiles[row]);
                if (!EventsDataHub->Events.isEmpty()) ui->fReloadRequired->setVisible(true);
              }
         }
        else if (selectedItem->iconText() == "Remove")
         {
            //remove
            if (row>-1)
              {
                delete ui->lwLoadedEventsFiles->item(row);
                LoadedEventFiles.removeAt(row);
                if (!EventsDataHub->Events.isEmpty()) ui->fReloadRequired->setVisible(true);
              }
         }
    }
}

void MainWindow::clearPreprocessingData()
{
    ui->cbPMsignalPreProcessing->setChecked(false);
}

void MainWindow::on_pbDeleteLoadedEvents_clicked()
{
  MainWindow::DeleteLoadedEvents(false);
  if (GeometryWindow->isVisible()) GeometryWindow->ShowGeometry(false);
}

void MainWindow::DeleteLoadedEvents(bool KeepFileList)
{
    QStringList tmpLA = LoadedEventFiles;
    QStringList tmpLT = LoadedTreeFiles;
    EventsDataHub->clear();

    ui->leoLoadedEvents->setText("");
    ui->leoTotalLoadedEvents->setText("");    
    ui->fReloadRequired->setVisible(false);
    if (KeepFileList)
      {
        LoadedEventFiles = tmpLA;
        for (int i=0; i<tmpLA.size(); i++)
          ui->lwLoadedEventsFiles->addItem(tmpLA.at(i));
        LoadedTreeFiles = tmpLT;
        for (int i=0; i<tmpLT.size(); i++)
          ui->lwLoadedSims->addItem(tmpLT.at(i));
      }
    else
      {
        //paranoic: probably not necessary now
        LoadedEventFiles.clear();
        ui->lwLoadedEventsFiles->clear();
        LoadedTreeFiles.clear();
        ui->lwLoadedSims->clear();
      }

    if (Detector->GeoManager) Detector->GeoManager->ClearTracks();    
    clearGeoMarkers();    
    Owindow->RefreshData();    
}

void MainWindow::on_pbPreprocessingLoad_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this,
                                                  "Load preprocessing data (ignores first column with PM number)", GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt)");

  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  MainWindow::LoadPreprocessingAddMulti(fileName);
  if (!EventsDataHub->Events.isEmpty()) ui->fReloadRequired->setVisible(true);
}

void MainWindow::on_pbPreprocessingSave_clicked()
{
  QString fileName = QFileDialog::getSaveFileName(this,
                                                  "Save preprocessing data", GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt)");

  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  MainWindow::SavePreprocessingAddMulti(fileName);
}

void MainWindow::on_sbPreprocessigPMnumber_valueChanged(int arg1)
{
  if ( (arg1>PMs->count()) || (arg1 == PMs->count() && !EventsDataHub->fLoadedEventsHaveEnergyInfo) )
    {
      if (arg1 == 0) return;
      ui->sbPreprocessigPMnumber->setValue(0);
      return;
    }  
  QString strA, strM;
  if (arg1 == PMs->count())
    {
      strA.setNum(TmpHub->PreEnAdd);
      strM.setNum(TmpHub->PreEnMulti);
    }
  else
    {
      strA.setNum(PMs->at(arg1).PreprocessingAdd);
      strM.setNum(PMs->at(arg1).PreprocessingMultiply);
    }
  ui->ledPreprocessingAdd->setText(strA);
  ui->lePreprocessingMultiply->setText(strM);
}

void MainWindow::on_cobPMshape_currentIndexChanged(int index)
{
  if (index == 1 || index == 3) ui->labSizeDiameterPM->setText("Diameter:");
  else ui->labSizeDiameterPM->setText("Size:");

  if (index == 0) ui->fRectangularPM->setVisible(true);
  else ui->fRectangularPM->setVisible(false);

  ui->frSphericalPm->setVisible( index == 3);
  ui->ledSizeZ->setDisabled(index == 3);
}

void MainWindow::on_pbElGainLoadDistr_clicked()
{
  int ipm = ui->sbElPMnumber->value();
  QString fileName;
  fileName = QFileDialog::getOpenFileName(this, "Load Single Photoelectron Pulse Height Spectrum", GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*)");
  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  QVector<double> x, y;
  bool error = MainWindow::LoadSPePHSfile(fileName, &x, &y);
  if (error == 0)
    {
      Detector->PMs->at(ipm).setElChanSPePHS(x, y);
      Detector->PMs->at(ipm).preparePHS();
    }
  else return;

  MainWindow::on_pbUpdateElectronics_clicked(); //reconstruct detector and update indication
}

void MainWindow::on_pbElGainShowDistr_clicked()
{  
  int ipm = ui->sbElPMnumber->value(); 
  PMs->at(ipm).preparePHS();
  GraphWindow->Draw(PMs->at(ipm).SPePHShist, "", true, false);
}

void MainWindow::on_pbElTestGenerator_clicked()
{
  int ipm = ui->sbElPMnumber->value();
  auto hist1D = new TH1D("tmpHistElTgen", "test", 100, 0, 0);
  for (int i=0; i<ui->sbElTestGeneratorEvents->value(); i++)
      hist1D->Fill(PMs->GenerateSignalFromOnePhotoelectron(ipm));
  GraphWindow->Draw(hist1D);
}

void MainWindow::on_sbElPMnumber_valueChanged(int arg1)
{
  if (arg1 == 0   &&   PMs->count() ==0) return;
  if (arg1 > PMs->count()-1)
    {
      if (PMs->count() == 0)
        {
          ui->sbElPMnumber->setValue(0);
          return;
        }
      ui->sbElPMnumber->setValue(PMs->count()-1);
      return;
    }
  MainWindow::on_pbElUpdateIndication_clicked();
}

void MainWindow::on_pbElUpdateIndication_clicked()
{
  //qDebug() << "----Electronics GUI update -----------";
  bool tmpBulk = BulkUpdate;
  BulkUpdate = true;

  ui->cbEnableSPePHS->setChecked(Detector->PMs->isDoPHS());
  ui->cbEnableMCcrosstalk->setChecked(Detector->PMs->isDoMCcrosstalk());
  ui->cbEnableElNoise->setChecked(Detector->PMs->isDoElNoise());
  ui->cbEnableADC->setChecked(Detector->PMs->isDoADC());
  ui->cbDarkCounts_Enable->setChecked(Detector->PMs->fDoDarkCounts);

  int ipm = ui->sbElPMnumber->value();
  int NumPMs = PMs->count();
  if (ipm>NumPMs-1 && ipm != 0)
    {
      ui->sbElPMnumber->setValue(0);
      return; //update on_change
    }

  if (NumPMs != 0)
    {
      ui->twElectronics->setEnabled(true);

      ui->labElType->setText(PMs->getType(PMs->at(ipm).type)->Name);
      QString str, str1;
      str.setNum( PMs->at(ipm).AverageSigPerPhE );
      ui->ledAverageSigPhotEl->setText(str);
      str.setNum(PMs->at(ipm).SPePHSsigma);
      ui->ledElsigma->setText(str);
      str.setNum(PMs->at(ipm).SPePHSshape);
      ui->ledElShape->setText(str);
      ui->pbElGainShowDistr->setEnabled( PMs->at(ipm).SPePHShist );

      //MCcrosstalk      
      ui->cobMCcrosstalk_Model->setCurrentIndex( PMs->at(ipm).MCmodel );
      if (PMs->at(ipm).MCmodel == 0)
      {
          ui->tabMCcrosstalk->clearContents();
          QVector<double> vals = PMs->at(ipm).MCcrosstalk;
          if (vals.isEmpty()) vals << 1.0;
          while (vals.size()<3) vals << 0;
          ui->tabMCcrosstalk->setColumnCount(vals.size());
          ui->tabMCcrosstalk->setRowCount(1);
          QStringList header;
          for (int i=0; i<vals.size(); i++)
            {
              QTableWidgetItem* item = new QTableWidgetItem(QString::number(vals.at(i)));
              item->setTextAlignment(Qt::AlignCenter);
              ui->tabMCcrosstalk->setItem(0, i, item);
              header << QString::number(i+1)+" ph.e.";
            }
          ui->tabMCcrosstalk->setVerticalHeaderLabels(QStringList("Probability"));
          ui->tabMCcrosstalk->setHorizontalHeaderLabels(header);
          ui->tabMCcrosstalk->resizeColumnsToContents();
          ui->tabMCcrosstalk->resizeRowsToContents();
      }
      else
      {
          ui->ledMCcrosstalkTriggerProb->setText( QString::number(PMs->at(ipm).MCtriggerProb) );

          double br = 1 - PMs->at(ipm).MCtriggerProb;
          double Epsilon = 1 - br*br*br*br;   //  1-epsilon = (1-p)^n    model with n = 4
          ui->labMCtotalProb->setText( QString::number(Epsilon, 'g', 4) );
          double np = 4.0*PMs->at(ipm).MCtriggerProb;
          double MCmeanCells = 1.0 + np + np*np;  // Average num triggered cells = 1 + np + (np)^2 + o(p^2)  model with n = 4
          ui->labMCmean->setText( QString::number(MCmeanCells, 'g', 4) );
      }

      ui->ledTimeOfOneMeasurement->setText( QString::number(PMs->at(ipm).MeasurementTime) );
      ui->swDarkCounts_Time->setCurrentIndex( ui->cbTimeResolved->isChecked() ? 1 : 0 );
      ui->cobDarkCounts_Model->setCurrentIndex( PMs->at(ipm).DarkCounts_Model );
      const bool bHaveDist = !PMs->at(ipm).DarkCounts_Distribution.isEmpty();
      ui->pbDarkCounts_Show->setEnabled(bHaveDist);
      ui->pbDarkCounts_Delete->setEnabled(bHaveDist);

      str.setNum(PMs->at(ipm).ElNoiseSigma);
      ui->ledElNoiseSigma->setText(str);
      str.setNum(PMs->at(ipm).ElNoiseSigma_StatSigma);
      ui->ledElNoiseSigma_Stat->setText(str);
      str.setNum(PMs->at(ipm).ElNoiseSigma_StatNorm);
      ui->ledElNoiseSigma_Norm->setText(str);

      //str.setNum(PMs->at(ipm).ADCmax);
      ui->ledADCmax->setText( QString::number(PMs->at(ipm).ADCmax) );
      ui->sbADCbits->setValue( PMs->at(ipm).ADCbits );

      str.setNum(PMs->at(ipm).ADClevels + 1);
      str = "levels: " + str;
      str1.setNum(PMs->at(ipm).ADCstep);
      str += "  signal/level: "+str1;
      ui->leoADCInfo->setText(str);

      const int& mode = PMs->at(ipm).SPePHSmode;
      if (mode == 3) ui->ledAverageSigPhotEl->setReadOnly(true);
      else ui->ledAverageSigPhotEl->setReadOnly(false);
      ui->cobPMampGainModel->setCurrentIndex(mode);
    }
  else ui->twElectronics->setEnabled(false);

  //ui->ledTimeOfOneMeasurement->setText( QString::number(Detector->PMs->at(ipm).MeasurementTime) );

  BulkUpdate = tmpBulk;
}

void MainWindow::on_pbElCopyGainData_clicked()
{
   int mode = ui->cobElCopyMode->currentIndex();
   if (mode == 0) return;
   int selector = ui->twElectronics->currentIndex(); //0-SPePHS, 1-crossTalk, 2-ElNoise, 3-ADC, 4-dark counts

   int ipm =  ui->sbElPMnumber->value();
   int typ = PMs->at(ipm).type;
   bool flagSelective = false;
   if (mode == 1) flagSelective = true;

   for (int ipmTo = 0; ipmTo < PMs->count(); ipmTo++)
     {
       if (ipmTo == ipm) continue;
       if (flagSelective)
         if (typ != PMs->at(ipmTo).type) continue;

       switch (selector)
         {
         case 0:
           PMs->at(ipmTo).copySPePHSdata(PMs->at(ipm));
           break;
         case 1:
           PMs->at(ipmTo).copyMCcrosstalkData(PMs->at(ipm));
           break;
         case 2:
           PMs->at(ipmTo).copyElNoiseData(PMs->at(ipm));
           break;
         case 3:
           PMs->at(ipmTo).copyDarkCountsData(PMs->at(ipm));
           break;
         case 4:
           PMs->at(ipmTo).copyADCdata(PMs->at(ipm));
           break;         
         default:
           qWarning() << "Unknown electronics selector";
         }
     }

   MainWindow::on_pbUpdateElectronics_clicked();
}

void MainWindow::on_cbEnableSPePHS_toggled(bool checked)
{
    ui->fElGains->setEnabled(checked);    
    if (checked) ui->twElectronics->setTabText(0, "Ph.e.PHS :On ");
    else ui->twElectronics->setTabText(0, "Ph.e.PHS :Off");
}

void MainWindow::on_cbEnableMCcrosstalk_toggled(bool checked)
{
  ui->frMCcrosstalk->setEnabled(checked);
  if (checked) ui->twElectronics->setTabText(1, "Cross-talk :On ");
  else ui->twElectronics->setTabText(1, "Cross-talk :Off");
}

void MainWindow::on_cbEnableElNoise_toggled(bool checked)
{
  ui->fElNoise->setEnabled(checked);
  if (checked) ui->twElectronics->setTabText(2, "Electronic noise :On ");
  else ui->twElectronics->setTabText(2, "Electronic noise :Off");
}

void MainWindow::on_cbDarkCounts_Enable_toggled(bool checked)
{
    ui->fDarkCounts->setEnabled(checked);
    if (checked) ui->twElectronics->setTabText(3, "Dark counts :On ");
    else ui->twElectronics->setTabText(3, "Dark counts :Off");
}

void MainWindow::on_cbEnableADC_toggled(bool checked)
{  
    ui->fADC->setEnabled(checked);    
    if (checked) ui->twElectronics->setTabText(4, "ADC :On ");
    else ui->twElectronics->setTabText(4, "ADC :Off");
}

void MainWindow::on_pbScanDistrLoad_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Load custom distribution", GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*)");
  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  MainWindow::LoadScanPhotonDistribution(fileName);
  MainWindow::on_pbUpdateSimConfig_clicked();
}

void MainWindow::on_pbScanDistrShow_clicked()
{
  if (!histScan) return;
  GraphWindow->Draw(histScan, "", true, false);
}

void MainWindow::on_pbScanDistrDelete_clicked()
{
  if (histScan)
    {
      delete histScan;
      histScan = 0;
    }
  ui->pbScanDistrShow->setEnabled(false);
  ui->pbScanDistrDelete->setEnabled(false);
  MainWindow::on_pbUpdateSimConfig_clicked();
}

void MainWindow::on_cobSecScintillationGenType_currentIndexChanged(int index)
{
  if (index == 3) ui->fSecondaryScintLoadProfile->setVisible(true);
  else ui->fSecondaryScintLoadProfile->setVisible(false);
}

void MainWindow::on_pbSecScintShowProfile_clicked()
{
  if (!histSecScint) return;
  GraphWindow->Draw(histSecScint, "", true, false);
}

void MainWindow::on_pbSecScintLoadProfile_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Load custom distribution", GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*)");
  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  MainWindow::LoadSecScintTheta(fileName);
}

void MainWindow::LoadSecScintTheta(QString fileName)
{
  QVector<double> x, y;
  int error = LoadDoubleVectorsFromFile(fileName, &x, &y);
  if (error>0)
    {
      qDebug()<<"Error reading custom Theta distribution file";
      return;
    }

  //PhotonGenerator->deleteSecScintThetaDistribution();
  if (histSecScint) delete histSecScint;
  int size = x.size();
  double* xx = new double [size];
  for (int i = 0; i<size; i++) xx[i]=x[i];//*3.1415926535/180.;
  histSecScint = new TH1D("SecScintTheta","SecScint: Theta", size-1, xx);

  for (int j = 1; j<size+1; j++)  histSecScint->SetBinContent(j, y[j-1]);
  //PhotonGenerator->setSecScintThetaDistribution(histSecScint);

  ui->pbSecScintShowProfile->setEnabled(true);
  ui->pbSecScintDeleteProfile->setEnabled(true);
}

void MainWindow::on_pbSecScintDeleteProfile_clicked()
{
  //PhotonGenerator->deleteSecScintThetaDistribution();
  if (histSecScint) delete histSecScint;
  histSecScint = 0;

  ui->pbSecScintShowProfile->setEnabled(false);
  ui->pbSecScintDeleteProfile->setEnabled(false);
  ui->cobSecScintillationGenType->setCurrentIndex(0);
}

void MainWindow::on_lwMaterials_currentRowChanged(int currentRow)
{
  if (currentRow<0) return;  
  ui->cobMaterialForOverrides->setCurrentIndex(currentRow);
  MainWindow::on_pbRefreshOverrides_clicked();
}

void MainWindow::on_lwMaterials_doubleClicked(const QModelIndex &index)
{
  ui->lwMaterials->setCurrentRow(index.row());
  MIwindow->showNormal();
  MIwindow->raise();
  MIwindow->activateWindow();
  MIwindow->SetMaterial(index.row());
}

void MainWindow::on_pbConfigureAddOns_clicked()
{
  //DAwindow->setForbidUpdateValues(false);
  DAwindow->show();
  DAwindow->raise();
  DAwindow->activateWindow();
}

void MainWindow::LoadSimTreeRequested()
{  
  QStringList fileNames = QFileDialog::getOpenFileNames(this, "Load/Append simulation data from Root tree", GlobSet.LastOpenDir, "Root files (*.root)");
  if (fileNames.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileNames.first()).absolutePath();

  if (LoadedTreeFiles.isEmpty()) MainWindow::ClearData(); //clear ascii loaded data  or sim if present
  else
    {
#ifdef ANTS_FLANN
   ReconstructionManager->KNNmodule->Filter.clear();
#endif
    }

  int numEvents = (ui->cbLimitNumberTreeEvents->isChecked() ? ui->sbFirstNeventsTree->value() : -1);

  for (int i=0; i<fileNames.size(); i++)
      MainWindow::LoadSimulationDataFromTree(fileNames[i], numEvents);
  Rwindow->OnEventsDataAdded();

  EventsDataHub->squeeze();
  //if (!LoadedTreeFiles.isEmpty()) ui->pbReloadTreeData->setEnabled(true);
}


void MainWindow::on_pbReloadTreeData_clicked()
{
    if (LoadedTreeFiles.isEmpty())
      {
        message("File list is empty!", this);
        MainWindow::DeleteLoadedEvents();
        return;
      }
    MainWindow::DeleteLoadedEvents(true); //keeping the FileList intact!

    int numEvents = (ui->cbLimitNumberTreeEvents->isChecked() ? ui->sbFirstNeventsTree->value() : -1);
    ui->lwLoadedSims->clear();

    QStringList files = LoadedTreeFiles;
    LoadedTreeFiles.clear();
    for (int i=0; i<files.size(); i++)
      {
        int loadedEvents = MainWindow::LoadSimulationDataFromTree(files[i], numEvents);
        ui->leoLoadedEvents->setText(QString::number(loadedEvents));
      }

    if (!EventsDataHub->Events.isEmpty())
      {
        Owindow->RefreshData();
        Rwindow->OnEventsDataAdded();
      }

    ui->leoTotalLoadedEvents->setText(QString::number(EventsDataHub->Events.size()));

    EventsDataHub->squeeze();
}

void MainWindow::timerTimeout()
{
    //qDebug() << "root in";
 //   RootApp->StartIdleing();
    //gSystem->InnerLoop(); - crashes!
    gSystem->ProcessEvents();
//    RootApp->StopIdleing();
    //qDebug() << "root out";
}

void MainWindow::on_ledPreprocessingAdd_editingFinished()
{
  int channel = ui->sbPreprocessigPMnumber->value();
  if ( (channel>PMs->count()) || (channel == PMs->count() && !EventsDataHub->fLoadedEventsHaveEnergyInfo) )
    {
      if (channel == 0) return;
      ui->sbPreprocessigPMnumber->setValue(0);
      return;
    }
  if (channel == PMs->count()) TmpHub->PreEnAdd = ui->ledPreprocessingAdd->text().toDouble();
  else PMs->at(channel).PreprocessingAdd = ui->ledPreprocessingAdd->text().toDouble();
  MainWindow::on_pbUpdatePreprocessingSettings_clicked();
}

void MainWindow::on_lePreprocessingMultiply_editingFinished()
{
  int channel = ui->sbPreprocessigPMnumber->value();
  if ( (channel>PMs->count()) || (channel == PMs->count() && !EventsDataHub->fLoadedEventsHaveEnergyInfo) )
    {
      if (channel == 0) return;
      ui->sbPreprocessigPMnumber->setValue(0);
      return;
    }

  QString text = ui->lePreprocessingMultiply->text();
  //parse for a simple ratio ("double"/"double")
  QStringList fields = text.split("/", QString::SkipEmptyParts);

  double finalValue = 1.0;
  if (fields.size() == 0 )
    {
      ui->lePreprocessingMultiply->setText("1");
      message("A value or ratio expected! Resetting to 1");
    }
  else if (fields.size() == 1)
    {
      bool ok;
      finalValue = text.toDouble(&ok);
      if (ok)
        {
          if (channel == PMs->count()) TmpHub->PreEnMulti = ui->lePreprocessingMultiply->text().toDouble();
          else PMs->at(channel).PreprocessingMultiply = ui->lePreprocessingMultiply->text().toDouble();
          MainWindow::on_pbUpdatePreprocessingSettings_clicked();
          return; //== done ==
        }
      else
        {
          ui->lePreprocessingMultiply->setText("1");
          message("A value or ratio expected! Resetting to 1");
          finalValue = 1.0;
        }      
    }
  else if (fields.size() == 2)
    {
       //processing ratio
       bool ok1, ok2;
       double top, bottom;
       top = fields[0].toDouble(&ok1);
       bottom = fields[1].toDouble(&ok2);
       if (!ok1 || !ok2 )
         {
           ui->lePreprocessingMultiply->setText("1");
           message("A value or ratio expected! Resetting to 1");
         }
       else if (bottom == 0)
         {
           ui->lePreprocessingMultiply->setText("1");
           message("Cannot divide by 0! Resetting to 1");
         }
       else finalValue = top / bottom;
    }

  ui->lePreprocessingMultiply->setText(QString::number(finalValue)); //to avoid rounding problems
  on_lePreprocessingMultiply_editingFinished(); //running again -> real exit is at ==done==
}

void MainWindow::on_extractPedestals_clicked()
{
   if (EventsDataHub->Events.isEmpty())
     {
       message("Data are not loaded!", this);
       return;
     }
   if (ui->cbLoadedDataHasEnergy->isChecked() && !EventsDataHub->fLoadedEventsHaveEnergyInfo)
     {
       message("select the energy channel and reload the data!");
       return;
     }

   int ibound = PMs->count();
   if (ui->cbLoadedDataHasEnergy->isChecked()) ibound++;

   //calculating average for each channel
   QVector<double> averageSignal(ibound, 0);
   for (int iev=0; iev < EventsDataHub->Events.count(); iev++)
     for (int ipm=0; ipm<ibound; ipm++)
         averageSignal[ipm] += EventsDataHub->Events[iev][ipm];

   QString str, line;
   for (int ipm=0; ipm<ibound; ipm++)
     {
       averageSignal[ipm] /= EventsDataHub->Events.size();
       str = QString::number(averageSignal[ipm], 'g', 4);
       line += str + " ";
     }
   Owindow->OutText("\nPedestals extracted:\n"+line+"\n");

   for (int ipm=0; ipm<PMs->count(); ipm++) PMs->at(ipm).PreprocessingAdd = -averageSignal[ipm];
   if (ui->cbLoadedDataHasEnergy->isChecked()) TmpHub->PreEnAdd = -averageSignal.last();
   MainWindow::on_sbPreprocessigPMnumber_valueChanged(ui->sbPreprocessigPMnumber->value());
   if (!EventsDataHub->Events.isEmpty()) ui->fReloadRequired->setVisible(true);
   MainWindow::on_pbUpdatePreprocessingSettings_clicked();
}

void MainWindow::on_sbLoadedEnergyChannelNumber_editingFinished()
{
    int i = ui->sbLoadedEnergyChannelNumber->value();

    if (i < PMs->count())
      {
        ui->sbLoadedEnergyChannelNumber->setValue(PMs->count());
        message("Channel number should be >= number of PMs", this);
        return;
      }
}

void MainWindow::on_cobPMtypes_currentIndexChanged(int index)
{
    if (DoNotUpdateGeometry) return;
    ui->sbPMtype->setValue(index);
}

void MainWindow::on_pbShowMaterialInPMtypes_clicked()
{
  int mat = ui->cobMatPM->currentIndex();
  MIwindow->show();
  MIwindow->raise();
  MIwindow->activateWindow();
  MIwindow->SetMaterial(mat);

  ui->lwMaterials->setCurrentRow(mat);
}

void MainWindow::on_cobPMtypeInArrays_currentIndexChanged(int index)
{
  if (DoNotUpdateGeometry) return;
  ui->sbPMtypeForGroup->setValue(index);
}

void MainWindow::on_pbSetPMtype_clicked()
{
  QList<int> ToSet;
  bool ok = ExtractNumbersFromQString(ui->lePMnumberForTypeSet->text(), &ToSet);
  if (!ok)
    {
      message("Input error in PM numbers!", this);
      return;
    }

  int counter = 0;
  int itype = ui->cobPMtypeInArraysForCustom->currentIndex();
  int ul = ui->cobUpperLowerPMs->currentIndex();
  for (int i=0; i<ToSet.size(); i++)
    {
      int ipm = ToSet[i];
      if (ipm<0 || ipm > PMs->count()-1) continue;
      if (PMs->at(ipm).upperLower != ul) continue;

      Detector->PMarrays[ul].PositionsAnglesTypes[ipm].type = itype;
      //PMs->at(ipm).type = itype;
      counter++;
    }

  message("Type has been changed for "+QString::number(counter)+" PMs", this);

  if (counter>0) MainWindow::ReconstructDetector();
}

void MainWindow::on_pbViewChangeRelQEfactors_clicked()
{
  MainWindow::ViewChangeRelFactors("QE");
}

void MainWindow::on_pbViewChangeRelELfactors_clicked()
{
  MainWindow::ViewChangeRelFactors("EL");
}

void MainWindow::ViewChangeRelFactors(QString options)
{
  QDialog* dialog = new QDialog(this);
  dialog->resize(350,380);
  dialog->move(this->x()+100, this->y()+100);

  if (options == "QE") dialog->setWindowTitle("Relative QE/PDE factors");
  else if (options == "EL") dialog->setWindowTitle("Relative factors of electronic channels");

  QPushButton *okButton = new QPushButton("Confirm");
  connect(okButton,SIGNAL(clicked()),dialog,SLOT(accept()));
  QPushButton *cancelButton = new QPushButton("Cancel");
  connect(cancelButton,SIGNAL(clicked()),dialog,SLOT(reject()));
  QHBoxLayout *buttonsLayout = new QHBoxLayout;
  buttonsLayout->addStretch(1);
  buttonsLayout->addWidget(okButton);
  buttonsLayout->addStretch(1);
  buttonsLayout->addWidget(cancelButton);
  buttonsLayout->addStretch(1);

  QTableWidget *tw = new QTableWidget();
  tw->clearContents();
  //tw->setShowGrid(false);
  int rows = PMs->count();
  tw->setRowCount(rows);

  int columns = 1;
  tw->setColumnCount(columns);
  if (options == "QE") tw->setHorizontalHeaderItem(0, new QTableWidgetItem("Relative QE/PDE"));
  else if (options == "EL") tw->setHorizontalHeaderItem(0, new QTableWidgetItem("Relative factors"));

  for (int i=0; i<rows; i++)
    {
      tw->setVerticalHeaderItem(i, new QTableWidgetItem("PM#"+QString::number(i)));
      if (options == "QE") tw->setItem(i, 0, new QTableWidgetItem(QString::number(PMs->at(i).relQE_PDE)));
      else if (options == "EL") tw->setItem(i, 0, new QTableWidgetItem(QString::number(PMs->at(i).AverageSigPerPhE)));
    }

  tw->setItemDelegate(new TableDoubleDelegateClass(tw)); //accept only doubles

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(tw);
  mainLayout->addLayout(buttonsLayout);

  dialog->setLayout(mainLayout);
  int result = dialog->exec();
  //qDebug()<<"dialog reports:"<<result;

  if (result == 1)
    {
      Detector->PMs->setDoPHS( true );  // *** should be in "EL" ?
      if (options == "QE")
        {
          //updating data
          for (int i=0; i<rows; i++) PMs->at(i).relQE_PDE = tw->item(i, 0)->text().toDouble();

          MainWindow::CalculateIndividualQEPDE();
          ReconstructDetector(true);
        }
      else if (options == "EL")
        {
          //updating data
          for (int i=0; i<rows; i++) PMs->at(i).relElStrength = tw->item(i, 0)->text().toDouble();

          PMs->CalculateElChannelsStrength();
          ui->cbEnableSPePHS->setChecked(true);
          ReconstructDetector(true);          
        }
    }
  delete dialog;  
}

void MainWindow::CalculateIndividualQEPDE()
{
  for (int ipm = 0; ipm<PMs->count(); ipm++)
    {
      double factor = PMs->at(ipm).relQE_PDE;
      int itype = PMs->at(ipm).type;

      //scalar value
      double fromType = PMs->getType(itype)->EffectivePDE;
      PMs->at(ipm).effectivePDE = fromType * factor;

      //Wavelength resolved data
      QVector<double> tmp = PMs->getType(itype)->PDE;
      QVector<double> tmp_lambda = PMs->getType(itype)->PDE_lambda;
      if (!tmp.isEmpty())
        {
          for (int i=0; i<tmp.size(); i++) tmp[i] *= factor;

          PMs->setPDEwave(ipm, &tmp_lambda, &tmp);
          PMs->RebinPDEsForPM(ipm);
        }
    }
}

void MainWindow::on_pbLoadRelQEfactors_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Load relative QE / PDE factors", GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*)");
  qDebug()<<fileName;

  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  Detector->PMs->setDoPHS( true );

  QVector<double> x;
  int ok = LoadDoubleVectorsFromFile(fileName, &x);

  if (ok != 0) return;
  if (x.size() != PMs->count())
    {
      message("Wrong number of PMs!", this);
      return;
    }

  for (int i=0; i<x.size(); i++) PMs->at(i).relQE_PDE = x[i];
  MainWindow::CalculateIndividualQEPDE();
  ReconstructDetector(true);
}

void MainWindow::on_pbClearRelQEfactors_clicked()
{
    for (int ipm = 0; ipm < PMs->count(); ipm++)
    {
        PMs->at(ipm).effectivePDE = -1.0;

        PMs->at(ipm).PDE.clear();
        PMs->at(ipm).PDE_lambda.clear();
        PMs->at(ipm).PDEbinned.clear();
    }
    ReconstructDetector(true);
}

void MainWindow::on_pbLoadRelELfactors_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Load relative strength of electronic channels", GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*)");
  qDebug()<<fileName;

  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  Detector->PMs->setDoPHS( true );

  QVector<double> x;
  int ok = LoadDoubleVectorsFromFile(fileName, &x);

  if (ok != 0) return;
  if (x.size() != PMs->count())
    {
      message("Wrong number of PMs!", this);
      return;
    }

  for (int i=0; i<x.size(); i++) PMs->at(i).relElStrength = x[i];
  PMs->CalculateElChannelsStrength();

  //ui->cbEnableSPePHS->setChecked(true);
  ReconstructDetector(true);
  //MainWindow::on_pbElUpdateIndication_clicked();
}

void MainWindow::on_pbRandomScaleELaverages_clicked()
{
  Detector->PMs->setDoPHS( true );

  bool bUniform = ( ui->cobScaleGainsUniNorm->currentIndex() == 0 );
  double min = ui->ledELavScaleMin->text().toDouble();
  double max = ui->ledELavScaleMax->text().toDouble();
  if (bUniform && min >= max) return;
  double mean = ui->ledELavScaleMean->text().toDouble();
  double sigma = ui->ledELavScaleSigma->text().toDouble();

  for (int ipm = 0; ipm<PMs->count(); ipm++)
    {
      double factor;
      if (bUniform)
        {
          factor = Detector->RandGen->Rndm();
          factor = min + (max-min)*factor;
        }
      else
          factor = Detector->RandGen->Gaus(mean, sigma);

      PMs->at(ipm).scaleSPePHS(factor);
    }

  ReconstructDetector(true);
}

void MainWindow::on_pbRelQERandomScaleELaverages_clicked()
{
    bool bUniform = ( ui->cobRelQEScaleGainsUniNorm->currentIndex() == 0 );
    double min = ui->ledRelQEELavScaleMin->text().toDouble();
    double max = ui->ledRelQEELavScaleMax->text().toDouble();
    if (bUniform && min >= max) return;
    double mean = ui->ledRelQEELavScaleMean->text().toDouble();
    double sigma = ui->ledRelQEELavScaleSigma->text().toDouble();

    for (int ipm = 0; ipm < PMs->count(); ipm++)
    {
        double factor;
        if (bUniform)
        {
            factor = Detector->RandGen->Rndm();
            factor = min + (max-min)*factor;
        }
        else
            factor = Detector->RandGen->Gaus(mean, sigma);

        PMs->at(ipm).relQE_PDE = factor;
    }

    CalculateIndividualQEPDE();
    ReconstructDetector(true);
}

void MainWindow::on_pbRelQESetELaveragesToUnity_clicked()
{
    double val = ui->ledRelQEvalue->text().toDouble();
    for (int ipm = 0; ipm < PMs->count(); ipm++)
        PMs->at(ipm).relQE_PDE = val;

    CalculateIndividualQEPDE();
    ReconstructDetector(true);
}

void MainWindow::on_pbSetELaveragesToUnity_clicked()
{
    Detector->PMs->setDoPHS( true );

    double val = ui->ledELEvalue->text().toDouble();
    for (int ipm = 0; ipm<PMs->count(); ipm++)
        PMs->at(ipm).scaleSPePHS(val);

    ReconstructDetector(true);
}

void MainWindow::on_pbShowRelGains_clicked()
{
  Owindow->show();
  Owindow->raise();
  Owindow->activateWindow();
  Owindow->SetTab(0);

  double max = -1e20;
  for (int ipm = 0; ipm<PMs->count(); ipm++)
    {
      double QE = PMs->at(ipm).effectivePDE;
      if (QE == -1.0) QE = PMs->getType( PMs->at(ipm).type )->EffectivePDE;
      double AvSig = 1.0;
      if (ui->cbEnableSPePHS->isChecked()) AvSig =  PMs->at(ipm).AverageSigPerPhE;
      double relStr = QE * AvSig;
      if (relStr > max) max = relStr;
    }
  if (fabs(max)<1.0e-20) max = 1.0;

  QVector<QString> tmp(0);
  Owindow->OutText("");
  Owindow->OutText("PM  Relative_QE   Average_signal_per_photoelectron   Relative_gain");
  for (int ipm = 0; ipm < PMs->count(); ipm++)
    {
      double QE = PMs->at(ipm).effectivePDE;
      if (QE == -1.0) QE = PMs->getType( PMs->at(ipm).type )->EffectivePDE;
      QString str = "PM#" + QString::number(ipm) +"> "+ QString::number(QE, 'g', 3);

      double AvSig = 1.0;
      if (ui->cbEnableSPePHS->isChecked()) AvSig =  PMs->at(ipm).AverageSigPerPhE;
      str += "  " + QString::number(AvSig, 'g', 3);

      double relStr = QE * AvSig / max;
      str += "  " + QString::number(relStr, 'g', 3);
      Owindow->OutText(str);
      tmp.append( QString::number(relStr, 'g', 3) );
    }

  GeometryWindow->ShowText(tmp, kRed);
}

void MainWindow::on_pbSaveResults_clicked()
{
    if (ui->cbSaveAsText->isChecked()) MainWindow::SaveSimulationDataAsText();
    else MainWindow::SaveSimulationDataTree();

    fSimDataNotSaved = false;
}

void MainWindow::SaveSimulationDataTree()
{
  if (EventsDataHub->Events.isEmpty())
    {
      message("No data to save!", this);
      return;
    }
  QString fileName = QFileDialog::getSaveFileName(this, "Save simulation data as Root Tree", GlobSet.LastOpenDir, "Root files (*.root)");
  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
  if(QFileInfo(fileName).suffix().isEmpty()) fileName += ".root";

  bool ok = EventsDataHub->saveSimulationAsTree(fileName);
  if (!ok) message("Error writing to file!", this);
}

void MainWindow::SaveSimulationDataAsText()
{
  if (EventsDataHub->Events.isEmpty())
    {
      message("No data to save!", this);
      return;
    }
  QString fileName = QFileDialog::getSaveFileName(this, "Save simulation data as text file", GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*.*)");
  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
  if(QFileInfo(fileName).suffix().isEmpty()) fileName += ".dat";

  bool ok = EventsDataHub->saveSimulationAsText(fileName, GlobSet.SimTextSave_IncludeNumPhotons, GlobSet.SimTextSave_IncludePositions);
  if (!ok) message("Error writing to file!", this);
}

void MainWindow::on_pbRemoveMaterial_clicked()
{
  int imat = ui->lwMaterials->currentRow();
  if (imat == -1)
    {
      message("Select a material to remove!", this);
      return;
    }
  TGeoVolume *vol;
  //check first that this material is in use
  TObjArray* list = Detector->GeoManager->GetListOfVolumes();
  int size = list->GetEntries();
  //qDebug() << "Num volumes:"<<size;
  bool found = false;
  for (int i=0; i<size; i++)
    {
      vol = (TGeoVolume*)list->At(i);
      //qDebug() << "index"<<i<<vol->GetName();
      //qDebug() <<  "   mat name:"<<vol->GetMaterial()->GetName()<< vol->GetMaterial()->GetIndex();
      if (!vol) break;
      if (imat == vol->GetMaterial()->GetIndex())
        {
          found = true;
          break;
        }
    }

  if (found)
  {
      message("Cannot remove this material - it is in use (volume: "+QString(vol->GetName())+")", this);
      return;
  }

  //previous test is obsolete now?
  //qDebug() << "checking World tree...";
  found = Detector->Sandwich->isMaterialInUse(imat);
  if (found)
  {
      message("Cannot remove this material - it is in use", this);
      return;
  }


  //ask for confirmation
  switch( QMessageBox::information( this, "Confirmation", "Delete material " + (*MpCollection)[imat]->name + "?","Remove", "Cancel",0, 1 ) )
    {
     case 0: //OK received;
        break;
     default:
      return;
    }
  //qDebug()<<"Removing!";

  //first updating detector elements, after it is finish, remove material from the collection (inverse order->crash)
    //Sandwich
  Detector->Sandwich->DeleteMaterial(imat);
  //qDebug() << "Remove mat: Sandwich updated";
   //PM types
  for (int itype=0; itype<PMs->countPMtypes(); itype++)
    {
      int tmat = PMs->getType(itype)->MaterialIndex;
      if (tmat > imat) PMs->getType(itype)->MaterialIndex = tmat-1;
    }
  //qDebug() << "Rem mat: PM types updated";
  //Dummy PMs use normal PM types - no need to update

   MpCollection->DeleteMaterial(imat);
   //qDebug() << "Rem mat: mat collection updated";

  MIwindow->SetMaterial(0);

  MainWindow::ReconstructDetector();

  //updating materials indication on mainwindow
  MainWindow::UpdateMaterialListEdit();
  ui->cobMaterialForOverrides->setCurrentIndex(0);
  MainWindow::on_pbRefreshOverrides_clicked(); //overrides are ready, just refresh viz  
}

void MainWindow::on_lwLoadedEventsFiles_itemChanged(QListWidgetItem *item)
{
  int row = ui->lwLoadedEventsFiles->currentRow();
  LoadedEventFiles[row] = item->text();
  if (!EventsDataHub->Events.isEmpty()) ui->fReloadRequired->setVisible(true);
}

void MainWindow::LRF_ModuleReadySlot(bool ready)
{
//   qDebug()<<ready;
   ui->fLRFs->setEnabled(ready);
   if (!ready && ui->cbLRFs->isChecked())
   {
       ui->cbLRFs->setChecked(false);
       MainWindow::on_pbUpdateSimConfig_clicked();
   }
}

void MainWindow::on_pbClearAdd_clicked()
{
  int i = QMessageBox::question( this, "Warning!", "Clear 'Add' values for all channels?", "Yes", "Cancel", 0, 1 );
  if (i == 1) return;
  for (int i=0; i<PMs->count(); i++) PMs->at(i).PreprocessingAdd = 0;
  TmpHub->PreEnAdd = 0;
  ui->sbPreprocessigPMnumber->setValue(0);
  MainWindow::on_sbPreprocessigPMnumber_valueChanged(0);
  MainWindow::on_pbUpdatePreprocessingSettings_clicked();
}

void MainWindow::on_pbClearMulti_clicked()
{
  int i = QMessageBox::question( this, "Warning!", "Clear 'Multiply'' values for all channels?", "Yes", "Cancel", 0, 1 );
  if (i == 1) return;
  for (int i=0; i<PMs->count(); i++) PMs->at(i).PreprocessingMultiply = 1.0;
  TmpHub->PreEnMulti = 1.0;
  ui->sbPreprocessigPMnumber->setValue(0);
  MainWindow::on_sbPreprocessigPMnumber_valueChanged(0);
  MainWindow::on_pbUpdatePreprocessingSettings_clicked();
}

void MainWindow::on_actionCredits_triggered()
{
  Credits cr(this);
  cr.exec();
}

void MainWindow::on_actionVersion_triggered()
{
  int minVer = ANTS2_MINOR;
  QString miv = QString::number(minVer);
  if (miv.length() == 1) miv = "0"+miv;
  int majVer = ANTS2_MAJOR;
  QString mav = QString::number(majVer);
  QString qv = QT_VERSION_STR;

  QString PythonVersion;
#ifdef __USE_ANTS_PYTHON__
  //PyObject *platform = PyImport_ImportModule("platform");
  //PyObject *python_version = PyObject_GetAttrString(platform, "python_version");
  //PyObject *version = PyObject_CallObject(python_version, NULL);
  //PythonVersion = PythonQtConv::PyObjGetString(version);

  PythonQtObjectPtr platform( PyImport_ImportModule("platform") );
  PythonQtObjectPtr python_version( PyObject_GetAttrString(platform.object(), "python_version") );
  PythonQtObjectPtr version( PyObject_CallObject(python_version.object(), NULL) );
  PythonVersion = PythonQtConv::PyObjGetString(version.object());
#endif

  QString out = "ANTS2\n"
                "   version:  " + mav + "." + miv + "\n"
                "   build date:  " + QString::fromLocal8Bit(__DATE__)+"\n"
                "\n"
                "Qt version:  " + qv + "\n"
                "\n"
                "ROOT version:  " + gROOT->GetVersion() + "\n"
                "\n"
                "Compilation options:"
                "\n"


#ifdef __USE_ANTS_CUDA__
  "  on"
#else
  "  off"
#endif
          " - CUDA (GPU-based reconstruction)\n"


#ifdef ANTS_FANN
  "  on"
#else
  "  off"
#endif
          " - FANN (neural networks)\n"

#ifdef ANTS_FLANN
  "  on"
#else
  "  off"
#endif
      " - FLANN (kNN searches)\n"

#ifdef USE_EIGEN
  "  on"
#else
  "  off"
#endif
     " - Eigen3 (fast LRF fitting)\n"

#ifdef USE_ROOT_HTML
 "  on"
#else
 "  off"
#endif
          " - Root html server (JSROOT visualization)  \n"

#ifdef __USE_ANTS_PYTHON__
 "  on - Python script (Python v." + PythonVersion + ")   "
#else
 "  off - Python script"
#endif
            "\n"

#ifdef __USE_ANTS_NCRYSTAL__
 "  on"
#else
 "  off"
#endif
            " - NCrystal library (neutron scattering)   \n"

                "";

  message(out, this);
}

void MainWindow::on_actionLicence_triggered()
{
  message("\nANTS2 is distributed under GNU Lesser General Public License version 3\n"
          "For details see http://www.fsf.org/", this);
}

void MainWindow::on_pnShowHideAdvanced_toggled(bool checked)
{
   if (checked) ui->swAdVancedSim->setCurrentIndex(1);
           else ui->swAdVancedSim->setCurrentIndex(0);
}

void MainWindow::on_pbYellow_clicked()
{
   bool fYellow = false;

   if (!ui->cbRandomDir->isChecked())
     {
       fYellow = true;
       ui->twAdvSimOpt->tabBar()->setTabIcon(0, Rwindow->YellowIcon);
     }
   else ui->twAdvSimOpt->tabBar()->setTabIcon(0, QIcon());

   if (ui->cbFixWavelengthPointSource->isChecked() && ui->cbWaveResolved->isChecked())
     {
       fYellow = true;
       ui->twAdvSimOpt->tabBar()->setTabIcon(1, Rwindow->YellowIcon);
     }
   else ui->twAdvSimOpt->tabBar()->setTabIcon(1, QIcon());

   if (ui->cbNumberOfRuns->isChecked())
     {
       fYellow = true;
       ui->twAdvSimOpt->tabBar()->setTabIcon(2, Rwindow->YellowIcon);
     }
   else ui->twAdvSimOpt->tabBar()->setTabIcon(2, QIcon());

   if (ui->cobScintTypePointSource->currentIndex() != 0 )
     {
       fYellow = true;
       ui->twAdvSimOpt->tabBar()->setTabIcon(3, Rwindow->YellowIcon);
     }
   else ui->twAdvSimOpt->tabBar()->setTabIcon(3, QIcon());

   ui->labAdvancedOn->setVisible(fYellow);
}

void MainWindow::on_pbReconstruction_clicked()
{
   Rwindow->showNormal();
   Rwindow->raise();
   Rwindow->activateWindow();
}

void MainWindow::on_pbReconstruction_2_clicked()
{
  Rwindow->showNormal();
  Rwindow->raise();
  Rwindow->activateWindow();
}

void MainWindow::on_pbSimulate_clicked()
{
  ELwindow->QuickSave(0);
  //ui->tabwidMain->setCurrentIndex(1);
  //ui->twSourcePhotonsParticles->setCurrentIndex(0);
  fStartedFromGUI = true;
  fSimDataNotSaved = false; // to disable the warning

  MainWindow::writeSimSettingsToJson(Config->JSON);  
  startSimulation(Config->JSON);  
}

void MainWindow::startSimulation(QJsonObject &json)
{
    WindowNavigator->BusyOn(); //go busy mode, most of gui controls disabled
    ui->pbStopScan->setEnabled(true);
    SimulationManager->StartSimulation(json, GlobSet.NumThreads, true);
}

void MainWindow::simulationFinished()
{
    //qDebug() << "---------Simulation finished. Events:"<<EventsDataHub->Events.size()<<"Was started from GUI:"<<fStartedFromGUI;
    if (fStartedFromGUI)
    {
        ui->pbStopScan->setEnabled(false);
        ui->pbStopScan->setText("stop");

        MIwindow->UpdateActiveMaterials(); //to update tmpMaterial

        if (!SimulationManager->isSimulationSuccess())
        {
            //qDebug() << "Sim manager reported fail!";
            ui->leEventsPerSec->setText("n.a.");
            if (!SimulationManager->isSimulationAborted())
                message(SimulationManager->getErrorString(), this);
        }

        bool showTracks = false;
        if (SimulationManager->bPhotonSourceSim)
        {
            showTracks = SimulationManager->TrackBuildOptions.bBuildPhotonTracks;
            clearGeoMarkers();
            Rwindow->ShowPositions(1, true);

            if (ui->twSingleScan->currentIndex() == 0 && SimulationManager->isSimulationSuccess())
                if (EventsDataHub->Events.size() == 1) Owindow->SiPMpixels = SimulationManager->SiPMpixels;
        }
        else
        {
            showTracks = SimulationManager->TrackBuildOptions.bBuildParticleTracks || SimulationManager->TrackBuildOptions.bBuildPhotonTracks;
            clearEnergyVector();
            EnergyVector = SimulationManager->EnergyVector; // TODO skip local EnergyVector?
            SimulationManager->EnergyVector.clear(); // to avoid clearing the energy vector cells
        }

        //prepare TGeoTracks
        if (showTracks)
        {
            int numTracks = 0;
            for (int iTr=0; iTr<SimulationManager->Tracks.size(); iTr++)
            {
                const TrackHolderClass* th = SimulationManager->Tracks.at(iTr);
                TGeoTrack* track = new TGeoTrack(1, th->UserIndex);
                track->SetLineColor(th->Color);
                track->SetLineWidth(th->Width);
                track->SetLineStyle(th->Style);
                for (int iNode=0; iNode<th->Nodes.size(); iNode++)
                    track->AddPoint(th->Nodes[iNode].R[0], th->Nodes[iNode].R[1], th->Nodes[iNode].R[2], th->Nodes[iNode].Time);
                if (track->GetNpoints()>1)
                {
                    numTracks++;
                    Detector->GeoManager->AddTrack(track);
                }
                else delete track;
            }
        }

        //Additional GUI updates
        if (GeometryWindow->isVisible())
        {
            GeometryWindow->ShowGeometry(false);
            if (showTracks) GeometryWindow->DrawTracks();
        }

        //qDebug() << "==>After sim: OnEventsDataLoadOrClear";
        Rwindow->OnEventsDataAdded();
          //qDebug()  << "==>Checked the available data, default Recon data created, basic filters applied";
        //Owindow->SetCurrentEvent(0);
        WindowNavigator->BusyOff(false);

        //warning for G4ants sim
        if (ui->twSourcePhotonsParticles->currentIndex()==1 && ui->cbGeant4ParticleTracking->isChecked() && SimulationManager->isSimulationSuccess())
        {
            QString s;
            double totalEdepo = SimulationManager->DepoByRegistered + SimulationManager->DepoByNotRegistered;
            if (totalEdepo == 0)
                s += "Total energy deposition in sensitive volumes is zero!\n\n";

            if (SimulationManager->DepoByNotRegistered > 0)
            {
                //double limit = 0.001;
                //if (SimulationManager->DepoByNotRegistered > limit * totalEdepo)
                {
                    s += QString("Deposition by not registered particles constitutes\n");
                    s += QString::number(100.0 * SimulationManager->DepoByNotRegistered / totalEdepo, 'g', 4) + " %";
                    s += "\nof the total energy deposition in the sensitive volume(s).\n\n";
                    s += "The following not registered particles were seen during Geant4 simulation:\n";
                    for (const QString & pn : SimulationManager->SeenNonRegisteredParticles)
                        s += pn + ", ";
                    s.chop(2);
                }
                //qDebug() << SimulationManager->SeenNonRegisteredParticles;
                message(s, this);
            }
        }
    }

    fStartedFromGUI = false;
    fSimDataNotSaved = true;
    //qDebug() << "---Procedure triggered by SimulationFinished signal has ended successfully---";
}

AParticleSourceSimulator *MainWindow::setupParticleTestSimulation(GeneralSimSettings &simSettings) //Single thread only!
{
    //============ prepare config ============
    QJsonObject json;
    SimGeneralConfigToJson(json);
    SimParticleSourcesConfigToJson(json);    
    json["Mode"] = "StackSim";
    json["DoGuiUpdate"] = true;
    //SaveJsonToFile(json, "ThisSimConfig.json");
    simSettings.readFromJson(json);
    simSettings.fLogsStat = true; //force to report logs

    //============  prepare  gui  ============
    WindowNavigator->BusyOn(); //go busy mode, most of gui controls disabled
    qApp->processEvents();

    //========== prepare simulator ==========
    AParticleSourceSimulator *pss = new AParticleSourceSimulator(Detector, SimulationManager, 0);

    pss->setSimSettings(&simSettings);
    //pss->setupStandalone(json);
    pss->setup(json);
    pss->initSimStat();
    pss->setRngSeed(Detector->RandGen->Rndm()*1000000);
    return pss;
}

void MainWindow::on_pbTrackStack_clicked()
{
    fSimDataNotSaved = false; // to disable the warning
    MainWindow::ClearData();
    GeneralSimSettings simSettings;
    AParticleSourceSimulator *pss = setupParticleTestSimulation(simSettings);
    EventsDataHub->SimStat->initialize(Detector->Sandwich->MonitorsRecords);

    //============ run stack =========
    bool fOK = pss->standaloneTrackStack(&ParticleStack);
      //qDebug() << "Standalone tracker reported:"<<fOK<<pss->getErrorString();

    //============   gui update   ============
    WindowNavigator->BusyOff();
    ui->pbStopScan->setEnabled(false);
    if (!fOK) message(pss->getErrorString(), this);
    else
    {
        //--- Retrieve results ---
        clearEnergyVector(); // just in case clear procedures change
        EnergyVector = pss->getEnergyVector();
        pss->ClearEnergyVectorButKeepObjects(); //disconnected this copy so delete of the simulator does not kill the vector
          //qDebug() << "-------------En vector size:"<<EnergyVector.size();

        //track handling
        //if (ui->cbBuildParticleTrackstester->isChecked())
        if (SimulationManager->TrackBuildOptions.bBuildParticleTracks)
          {
            //int numTracks = 0;
              //qDebug() << "Tracks collected:"<<pss->tracks.size();
            for (int iTr=0; iTr<pss->tracks.size(); iTr++)
            {
                const TrackHolderClass* th = pss->tracks[iTr];

                //if (numTracks<GlobSet.MaxNumberOfTracks)
                //{
                    TGeoTrack* track = new TGeoTrack(1, th->UserIndex);
                    track->SetLineColor(th->Color);
                    track->SetLineWidth(th->Width);
                    track->SetLineStyle(th->Style);
                    for (int iNode=0; iNode<th->Nodes.size(); iNode++)
                      track->AddPoint(th->Nodes[iNode].R[0], th->Nodes[iNode].R[1], th->Nodes[iNode].R[2], th->Nodes[iNode].Time);

                    if (track->GetNpoints()>1)
                      {
                        //numTracks++;
                        Detector->GeoManager->AddTrack(track);
                      }
                    else delete track;
                //}
                delete th;
            }
            pss->tracks.clear();
          }
        //if tracks are visible, show them
        if (GeometryWindow->isVisible())
        {
            GeometryWindow->ShowGeometry();
            if (SimulationManager->TrackBuildOptions.bBuildParticleTracks) GeometryWindow->DrawTracks();
        }
        //report data saved in history
        pss->appendToDataHub(EventsDataHub);
          //qDebug() << "Event history imported";

        ui->pbGenerateLight->setEnabled(true);
        Owindow->SetCurrentEvent(0);
    }
    delete pss;
}

void MainWindow::on_pbGenerateLight_clicked()
{
    fSimDataNotSaved = false; // to disable the warning
    MainWindow::ClearData();
    GeneralSimSettings simSettings;
    AParticleSourceSimulator *pss = setupParticleTestSimulation(simSettings);
    Detector->PMs->configure(&simSettings); //also configures accelerators!!!
    bool fOK = pss->standaloneGenerateLight(&EnergyVector);

    //============   gui update   ============
    WindowNavigator->BusyOff();
    ui->pbStopScan->setEnabled(false);
    if (!fOK) message(pss->getErrorString(), this);
    else
    {
        //---- Retrieve results ----
        //EnergyVector = SPaS.getEnergyVector();  no need for energyVector in this case!
        //EventHistory = SPaS.getEventHistory();
        //EventsDataHub->GeneratedPhotonsHistory = SPaS.getGeneratedPhotonsHistory();
        //GenHistory is in EventsDataHub

        if (GeometryWindow->isVisible())
        {
            GeometryWindow->ShowGeometry();
            if (SimulationManager->TrackBuildOptions.bBuildParticleTracks) GeometryWindow->DrawTracks();
        }
        pss->appendToDataHub(EventsDataHub);
        //Owindow->SetCurrentEvent(0);
        //Owindow->ShowGeneratedPhotonsLog();
        Rwindow->OnEventsDataAdded();
    }
    delete pss;
}

void MainWindow::RefreshOnProgressReport(int Progress, double msPerEv)
{
  ui->prScan->setValue(Progress);
  WindowNavigator->setProgress(Progress);
  ui->leEventsPerSec->setText( (msPerEv==0) ? "n.a." : QString::number(msPerEv, 'g', 4));

  qApp->processEvents();
}

void MainWindow::on_pbGDML_clicked()
{
  MainWindow::on_pbConfigureAddOns_clicked();
  DAwindow->SetTab(2);
}

void MainWindow::on_pbSavePMtype_clicked()
{
  QString starter = (GlobSet.LibPMtypes.isEmpty()) ? GlobSet.LastOpenDir : GlobSet.LibPMtypes;
  starter += "/PMtype_"+ui->lePMtypeName->text();

  QFileDialog fileDialog;
  QString fileName = fileDialog.getSaveFileName(this, "Save PM type properties", starter, "Json files (*.json);; All files (*.*)");
  if (fileName.isEmpty()) return;
  QFileInfo file(fileName);
  if (file.suffix().isEmpty()) fileName += ".json";

  int index = ui->sbPMtype->value();
  QJsonObject json;
  PMs->getType(index)->writeToJson(json); //here material is written as index!
    //adding material
  int imat = PMs->getType(index)->MaterialIndex;
  QJsonObject jsmat;
  MpCollection->writeMaterialToJson(imat, jsmat);
  json["Material"] = jsmat;
  bool bOK = SaveJsonToFile(json, fileName);
  if (!bOK) message("Failed to save json to file: "+fileName, this);
}

void MainWindow::on_pbLoadPMtype_clicked()
{
  QString starter = (GlobSet.LibPMtypes.isEmpty()) ? GlobSet.LastOpenDir : GlobSet.LibPMtypes;
  QFileDialog fileDialog;
  QString fileName = fileDialog.getOpenFileName(this, "Load PM type properties", starter, "Json files (*.json);; All files (*.*)");
  if (fileName.isEmpty()) return;

  QJsonObject json;
  bool bOK = LoadJsonFromFile(json, fileName);
  if (!bOK)
  {
      message("Cannot open file: "+fileName, this);
      return;
  }
  if (json.isEmpty())
    {
      message("Wrong file format: Json is empty!");
      return;
    }
  if (!json.contains("Material"))
    {
      message("Frong json format!");
      return;
    }

  QJsonObject jsmat = json["Material"].toObject();
  MpCollection->AddNewMaterial(jsmat);
  int newMatIndex = MpCollection->countMaterials()-1;
  QString matname = (*MpCollection)[newMatIndex]->name;

  APmType* typ = new APmType();
  typ->readFromJson(json);
  typ->MaterialIndex = newMatIndex;
  PMs->appendNewPMtype(typ);

  message("New PM type "+ typ->Name +" added and a new material "+matname+" for the optical interface was registered", this);

  PMs->RecalculateAngular();
  PMs->RebinPDEs();
  MainWindow::on_pbRefreshPMproperties_clicked(); //to refresh indication (also buttons enable/disable)

  MainWindow::ReconstructDetector(); //rebuild detector
}

void MainWindow::on_actionOpen_settings_triggered()
{   
   GlobSetWindow->showNormal();
   GlobSetWindow->activateWindow();
}

void MainWindow::on_actionSave_Load_windows_status_on_Exit_Init_toggled(bool arg1)
{
   GlobSet.SaveLoadWindows = arg1;
}

void MainWindow::on_pbUpdateElectronics_clicked()
{
   PMs->setDoPHS( ui->cbEnableSPePHS->isChecked() );
   PMs->setDoMCcrosstalk( ui->cbEnableMCcrosstalk->isChecked() );
   PMs->setDoElNoise( ui->cbEnableElNoise->isChecked() );
   PMs->setDoADC( ui->cbEnableADC->isChecked() );
   PMs->fDoDarkCounts = ui->cbDarkCounts_Enable->isChecked();

   const int ipm = ui->sbElPMnumber->value();

   PMs->at(ipm).SPePHSmode       = ui->cobPMampGainModel->currentIndex();
   PMs->at(ipm).AverageSigPerPhE = ui->ledAverageSigPhotEl->text().toDouble();
   PMs->at(ipm).SPePHSsigma      = ui->ledElsigma->text().toDouble();
   PMs->at(ipm).SPePHSshape      = ui->ledElShape->text().toDouble();

   PMs->at(ipm).ElNoiseSigma     = ui->ledElNoiseSigma->text().toDouble();
   PMs->at(ipm).ElNoiseSigma_StatSigma = ui->ledElNoiseSigma_Stat->text().toDouble();
   PMs->at(ipm).ElNoiseSigma_StatNorm  = ui->ledElNoiseSigma_Norm->text().toDouble();

   PMs->at(ipm).setADC(ui->ledADCmax->text().toDouble(), ui->sbADCbits->value());

   PMs->at(ipm).MCmodel          = ui->cobMCcrosstalk_Model->currentIndex();
   PMs->at(ipm).MCtriggerProb    = ui->ledMCcrosstalkTriggerProb->text().toDouble();

   PMs->at(ipm).MeasurementTime  = ui->ledTimeOfOneMeasurement->text().toDouble();
   PMs->at(ipm).DarkCounts_Model = ui->cobDarkCounts_Model->currentIndex();

   ReconstructDetector(true); //GUI update is triggered automatically
}

void MainWindow::on_pbOverlay_clicked()
{
  if (EventsDataHub->isEmpty())
    {
      message("There are no data!", this);
      return;
    }

  QString fileName = QFileDialog::getOpenFileName(this, "Overlay simulation data with data from an ASCII file", GlobSet.LastOpenDir, "All file (*.*)");
  if (fileName.isEmpty()) return;
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  WindowNavigator->BusyOn();
  bool ok = EventsDataHub->overlayAsciiFile(fileName, ui->cbApplyAddMultiplyPreprocess->isChecked(), Detector->PMs);
  if (!ok)
      message("Overlay error: "+EventsDataHub->ErrorString, this);
  EventsDataHub->clearReconstruction();
  Owindow->RefreshData();
  ui->lwLoadedSims->clear();
  WindowNavigator->BusyOff(false);
}

void MainWindow::on_pbLoadManifestFile_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Load manifest file", GlobSet.LastOpenDir, "Text files (*.txt);;All files (*)");
  if (fileName.isEmpty()) return;
  //this->activateWindow();
  //this->raise();
  GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  ui->leManifestFile->setText(fileName);
  ui->cbLoadedDataHasPosition->setChecked(false);
  if (!EventsDataHub->Events.isEmpty()) ui->fReloadRequired->setVisible(true);

  MainWindow::on_pbUpdatePreprocessingSettings_clicked();
}

void MainWindow::on_pbDeleteManifestFile_clicked()
{  
  ui->leManifestFile->setText("");
  if (!EventsDataHub->Events.isEmpty()) ui->fReloadRequired->setVisible(true);
  MainWindow::on_pbUpdatePreprocessingSettings_clicked();
}

void MainWindow::on_pbManifestFileHelp_clicked()
{
    QString str = "ASCII file giving true positions for experimental data\n\n"
                  "Format: file_name   X_position   Y_position \n"
                  "    or: file_name   X_position   Y_position   Events_to_load\n"
                  "---------------\n"
                  "Command lines (define the handling of the data files located after the command):\n"
                  "#type hole   -> set round irradiated area\n"
                  "#type slit   -> set slit shaped irradiated area\n"
                  "#X0 double    -> double = offset of zero in X direction\n"
                  "#Y0 double    -> double = offset of zero in Y direction\n"
                  "#dX double   -> double = step in X direction in mm\n"
                  "#dY double   -> double = step in Y direction in mm\n"
                  "hole-type specific:\n"
                  "#Diameter double   -> double = diameter of irradiated area in mm\n"
                  "slit-type specific:\n"
                  "#Angle double   -> double = angle of slit with X axis in degrees\n"
                  "#Width double   -> double = slit width in mm\n"
                  "#Length double  -> double = slit length in mm\n"
                  "---------------\n"
                  "Comment line should start from \\\\ \n"
                  "Empty lines allowed.\n";
    message(str, this);
}

void MainWindow::on_pbLoadAppendFiles_clicked()
{ 
   WindowNavigator->BusyOn();
   qApp->processEvents();

   if (ui->cobLoadDataType->currentIndex() == 0) LoadPMsignalsRequested();
   else LoadSimTreeRequested();

   WindowNavigator->BusyOff();
}

void MainWindow::on_sbLoadASCIIpositionXchannel_valueChanged(int arg1)
{
   ui->sbLoadASCIIpositionYchannel->setValue(arg1+1);
}

void MainWindow::on_pbPDEFromWavelength_clicked()
{
    //int prMat = Detector->Sandwich.Layers[Detector->Sandwich.ZeroLayer].Material; connot do it this way, geometry could be loaded from a GDML file!

    //finding the material index of PrScint
    TObjArray* vols = Detector->GeoManager->GetListOfVolumes();
    TGeoVolume* PrScint = (TGeoVolume*)vols->FindObject("PrScint");
    if (!PrScint)
      {
        message("Primary scintillator not found!", this);
        return;
      }
    //finding material
    QString matName = PrScint->GetMaterial()->GetName();
    qDebug() << "PrScint has material with name:"<<matName;
    int iMat = Detector->MpCollection->FindMaterial(matName);
    if (iMat == -1 )
      {
        message("Material of PrScint volume not found!", this);
        return;
      }
    qDebug() << "PrScint material index is "<< iMat;

    if ((*Detector->MpCollection)[iMat]->PrimarySpectrum.isEmpty())
      {
        message("Primary scintillation emission spectrum is not defined for the material of the primary scintillator ('PrScint') volume!\n"
                "Check material "+matName, this);
        return;
      }
    if (PMs->getType(ui->sbPMtype->value())->PDE.isEmpty())
      {
        message("Wavelength-resolved data are not defined for this PM type!", this);
        return;
      }

    //  const QVector<double> &prSpectrum = (*Detector->MaterialCollection)[prMat]->PrimarySpectrum; cannot do it this way - the data can have different binning!
    //  const QVector<double> &pmTypePDE = PMs->getType(ui->sbPMtype->value())->PDE;

    qDebug() << "Converting data to standart wavelength: From To Nodes"<<WaveFrom<<WaveTo<<WaveNodes;
    QVector<double> prSpectrum;
    ConvertToStandardWavelengthes(&(*Detector->MpCollection)[iMat]->PrimarySpectrum_lambda,
                                  &(*Detector->MpCollection)[iMat]->PrimarySpectrum,
                                  WaveFrom, WaveStep, WaveNodes,
                                  &prSpectrum);
    QVector<double> pmTypePDE;
    ConvertToStandardWavelengthes(&PMs->getType(ui->sbPMtype->value())->PDE_lambda,
                                  &PMs->getType(ui->sbPMtype->value())->PDE,
                                  WaveFrom, WaveStep, WaveNodes,
                                  &pmTypePDE);

    double weightedSum = 0, sum = 0;
    for(int i = 0; i < prSpectrum.size(); i++)
      {
        weightedSum += prSpectrum[i] * pmTypePDE[i];
        sum += prSpectrum[i];
      }

    if (sum == 0)
      {
        message("Normalisation problem: sum = 0", this);
        return;
      }

    ui->ledPDE->setText(QString::number(weightedSum/sum, 'g', 4));
    on_pbUpdatePMproperties_clicked(); //update detector
}

void MainWindow::on_cobLoadDataType_customContextMenuRequested(const QPoint &/*pos*/)
{
    if (ui->cobLoadDataType->currentIndex() == 0) ui->cobLoadDataType->setCurrentIndex(1);
    else ui->cobLoadDataType->setCurrentIndex(0);
}

void MainWindow::on_pbManuscriptExtractNames_clicked()
{
   QString manifest = ui->leManifestFile->text();
   if (manifest.isEmpty()) return;

   MainWindow::DeleteLoadedEvents();

   QFile file(manifest);
   if(!file.open(QIODevice::ReadOnly | QFile::Text))
     {
       qWarning() << "||| Could not open manifest file!";
       return;
     }
   QString path = QFileInfo(file).absolutePath();
     //qDebug() << "Path to manifest file:"<<path;

   QTextStream in(&file);
   QRegExp rx("(\\ |\\t)");

   while(!in.atEnd())
    {
      QString line = in.readLine().trimmed();
      if ( line.startsWith("//") || line.startsWith("#")) continue; //this is a comment or control, skip
      QStringList fields = line.split(rx, QString::SkipEmptyParts);
      if (fields.isEmpty()) continue; //empty line
      if (fields.size()<3)
        {
          qWarning() << "Bad format (too short) of manifest item:"<<line;
          return;
        }

      QString fn = path + "/" + fields[0];
      LoadedEventFiles.append(fn);
      ui->lwLoadedEventsFiles->addItem(fn);
     }

   //ui->fReloadRequired->setVisible(true);
   //ui->pbReloadExpData->setEnabled(!LoadedEventFiles.isEmpty());

   qApp->processEvents();
   MainWindow::on_pbReloadExpData_clicked();
}

void MainWindow::setFloodZposition(double Z)
{
  ui->ledScanFloodZ->setText(QString::number(Z));
}

void MainWindow::on_pbUpdatePreprocessingSettings_clicked()
{
    writeLoadExpDataConfigToJson(Detector->PreprocessingJson);
    Detector->writeToJson(Config->JSON);

    if (!EventsDataHub->Events.isEmpty()) ui->fReloadRequired->setVisible(true);

    if (GenScriptWindow && GenScriptWindow->isVisible())
        GenScriptWindow->updateJsonTree();
    if (ScriptWindow && ScriptWindow->isVisible())
        ScriptWindow->updateJsonTree();
}

void MainWindow::on_pbUpdateSimConfig_clicked()
{
    writeSimSettingsToJson(Config->JSON);

    // reading back - like with the detector; if something is not saved, will be obvious
    readSimSettingsFromJson(Config->JSON);
    Detector->Config->UpdateSimSettingsOfDetector();

    UpdateTestWavelengthProperties();

    if (ScriptWindow && ScriptWindow->isVisible())
        ScriptWindow->updateJsonTree();

    on_pbElUpdateIndication_clicked(); //Dark noise indication might change: depends on time-resolved status
}

void MainWindow::on_pbStopLoad_clicked()
{
    fStopLoadRequested = true;
    emit RequestStopLoad();
}

void MainWindow::on_pbConfigureNumberOfThreads_clicked()
{
    GlobSetWindow->show();
    GlobSetWindow->raise();
    GlobSetWindow->activateWindow();
    GlobSetWindow->SetTab(1);
}

void MainWindow::on_cobFixedDirOrCone_currentIndexChanged(int index)
{
    ui->fConeForPhotonGen->setEnabled(index==1);
}

void MainWindow::on_pbLockGui_clicked()
{
    onGuiEnableStatus(true);
    writeExtraGuiToJson(Config->JSON);
}

void MainWindow::on_pbUnlockGui_clicked()
{
    onGuiEnableStatus(false);
    writeExtraGuiToJson(Config->JSON);
}

void MainWindow::on_actionNewLRFModule_triggered()
{
  newLrfWindow->showNormal();
  newLrfWindow->raise();
  newLrfWindow->activateWindow();
}

void MainWindow::onGuiEnableStatus(bool fLocked)
{
  fConfigGuiLocked = fLocked;
  ui->pbLockGui->setEnabled(!fLocked);
  ui->pbUnlockGui->setVisible(fLocked);
  ui->tabWidget_2->setEnabled(!fLocked);
  ui->tabWidget->setEnabled(!fLocked);
}

void MainWindow::on_cobPMampGainModel_currentIndexChanged(int index)
{
    ui->ledAverageSigPhotEl->setEnabled(index !=3 );
}

void MainWindow::on_pbAddCellMCcrosstalk_clicked()
{
  BulkUpdate = true;

  int now = ui->tabMCcrosstalk->columnCount();
  ui->tabMCcrosstalk->setColumnCount(now+1);

  QTableWidgetItem* item = new QTableWidgetItem("0");
  item->setTextAlignment(Qt::AlignCenter);
  ui->tabMCcrosstalk->setItem(0, now, item);

  QStringList header;
  for (int i=0; i<now+1; i++)
      header << QString::number(i+1)+" ph.e.";
  ui->tabMCcrosstalk->setHorizontalHeaderLabels(header);
  ui->tabMCcrosstalk->resizeColumnsToContents();
  ui->tabMCcrosstalk->resizeRowsToContents();

  BulkUpdate = false;
}

void MainWindow::on_pbRemoveCellMCcrosstalk_clicked()
{
    QMessageBox msgBox;
    msgBox.setText("Reset crosstalk propoerties for this PM?");
    //msgBox.setInformativeText("Reset?");
    msgBox.setStandardButtons(QMessageBox::Reset | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    if (msgBox.exec() != QMessageBox::Reset) return;

    int ipm = ui->sbElPMnumber->value();
    PMs->at(ipm).MCcrosstalk = (QVector<double>() << 1.0);
    on_pbUpdateElectronics_clicked();
}

void MainWindow::on_pbMCnormalize_clicked()
{
   int ipm = ui->sbElPMnumber->value();
   QVector<double> vals = PMs->at(ipm).MCcrosstalk;

   if (vals.size()<=1)
     {
       vals.clear();
       vals << 1.0;
     }
   else
     {
       double sum = 0;
       for (int i=0; i<vals.size(); i++) sum += vals.at(i);

       if (sum<=0)
         {
           message("Sum of all values has to be > 0. Resetting values!", this);
           vals.clear();
           vals << 1.0;
         }
       else
         {
           for (int i=0; i<vals.size(); i++)
             vals[i] /= sum;
         }
     }

   PMs->at(ipm).MCcrosstalk = vals;
   on_pbUpdateElectronics_clicked();
}

void MainWindow::on_pbShowMCcrosstalk_clicked()
{
  PMs->prepareMCcrosstalk();

  int ipm = ui->sbElPMnumber->value();
  TH1I* hist = new TH1I("aa", "Test custom sampling", GlobSet.BinsX, 0, 0);
  for (int i=0; i<1000000; i++)
      hist->Fill(PMs->at(ipm).MCsampl->sample(Detector->RandGen)+1);

  GraphWindow->Draw(hist);
}

void MainWindow::on_pbLoadMCcrosstalk_clicked()
{
    int ipm = ui->sbElPMnumber->value();
    QString fileName;
    fileName = QFileDialog::getOpenFileName(this, "Load MC cross-talk distribution\nFile should contain a single column of values, starting with the probability for single event.\nData may be not normalized.", GlobSet.LastOpenDir, "Data files (*.dat *.txt);;All files (*)");
    if (fileName.isEmpty()) return;
    GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

    QVector<double> v;
    bool error = LoadDoubleVectorsFromFile(fileName, &v);

    if (error == 0) PMs->at(ipm).MCcrosstalk = v;
    else return;
    MainWindow::on_pbUpdateElectronics_clicked(); //reconstruct detector and update indication
}

void MainWindow::on_tabMCcrosstalk_cellChanged(int row, int column)
{
    if (BulkUpdate) return;

      //qDebug() << "Cell changed!" << row << column;

    bool ok;
    QString str = ui->tabMCcrosstalk->item(row,column)->text();
    double val = str.toDouble(&ok);
    if (!ok || val<0)
      {
        message("Input error: non-negative values only!", this);
        on_pbElUpdateIndication_clicked();
        return;
      }

    QVector<double> vals;
    double sum = 0;
    for (int i=0; i<ui->tabMCcrosstalk->columnCount(); i++)
      {
        double val = 0;
        QString str = ui->tabMCcrosstalk->item(row, i)->text();
          //qDebug() << i << str;
        val = str.toDouble();
        vals << val;
        sum += val;
      }
      //qDebug() << "Vector:"<<vals;

    if (sum == 0)
      {
        message("Sum of all values cannot be zero!", this);
        on_pbElUpdateIndication_clicked();
        return;
      }

    PMs->at(ui->sbElPMnumber->value()).MCcrosstalk = vals;
    on_pbUpdateElectronics_clicked();
}

void MainWindow::on_leLimitNodesObject_textChanged(const QString &/*arg1*/)
{
    bool fFound = (ui->cbLimitNodesOutsideObject->isChecked()) ?
         Detector->Sandwich->isVolumeExist(ui->leLimitNodesObject->text()) : true;

    QPalette palette = ui->leLimitNodesObject->palette();
    palette.setColor(QPalette::Text, (fFound ? Qt::black : Qt::red) );
    ui->leLimitNodesObject->setPalette(palette);
}

void MainWindow::on_cbLimitNodesOutsideObject_toggled(bool /*checked*/)
{
    on_leLimitNodesObject_textChanged("");
}

void MainWindow::on_bpResults_clicked()
{
   Owindow->show();
   Owindow->raise();
   Owindow->activateWindow();
   Owindow->SetTab(1);
}

void MainWindow::ShowGeometrySlot()
{
    GeometryWindow->ShowGeometry(false, false);
}

void MainWindow::on_bpResults_2_clicked()
{
  Owindow->show();
  Owindow->raise();
  Owindow->activateWindow();
  Owindow->SetTab(3);
}

void MainWindow::on_cobPartPerEvent_currentIndexChanged(int index)
{
    QString s;
    if (index == 0) s = "# of particles per event:";
    else            s = "average particles per event:";
    ui->labPartPerEvent->setText(s);
}

void MainWindow::on_ledElNoiseSigma_Norm_editingFinished()
{
    double newVal = ui->ledElNoiseSigma_Norm->text().toDouble();
    if (newVal < 1.0e-10)
    {
        newVal = 1.0;
        ui->ledElNoiseSigma_Norm->setText("1.0");
        message("Value has to be > 0", this);
    }
    on_pbUpdateElectronics_clicked();
}

void MainWindow::on_pbDarkCounts_Show_clicked()
{
    const int ipm = ui->sbElPMnumber->value();
    if (ipm >= PMs->count())
    {
        message("Bad PM index", this);
        return;
    }

    const QVector<double>& dist = PMs->at(ipm).DarkCounts_Distribution;
    if (dist.isEmpty())
    {
        message("Distribution is not defined", this);
        return;
    }

    TString title = "Generator distribution for PM #";
    title += ipm;
    QVector<double> x;
    for (int i = 0; i < dist.size(); i++) x << i;
    TGraph* g = GraphWindow->ConstructTGraph(x, dist, title, "", "");
    GraphWindow->Draw(g, "AP");
}

void MainWindow::on_pbDarkCounts_Load_clicked()
{
    const int ipm = ui->sbElPMnumber->value();
    if (ipm >= PMs->count())
    {
        message("Bad PM index", this);
        return;
    }

    const QString fileName = QFileDialog::getOpenFileName(this, QString("Load generation distribution for PM #").arg(ipm), GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*)");
    if (fileName.isEmpty()) return;
    GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

    QVector<double> x;
    int res = LoadDoubleVectorsFromFile(fileName, &x);
    if (res == 0)
    {
        qDebug() << "Original:\n"<< x;
        const int LoadOption = ui->cobDarkCounts_LoadOptions->currentIndex(); //0=as is; 1=scale; 2=Integrate; 3=Integrate then scale
        if (LoadOption == 2 || LoadOption == 3)
        {
            // integrating
            int range = ui->sbDarkCounts_IntegralRange->value();
            if (range == 0)
                range = x.size();
            else if (range > x.size())
            {
                message("Error: given range is larger than the length of the distribution in the file!", this);
                return;
            }

            QVector<double> xi;
            int start = 0;
            int inWindow = 1; // max inWindow vaue = range; on right side clipping is in the sum cycle condition
            while (start < x.size())
            {
                double val = 0;
                for (int i = start; (i < (start + inWindow)) && (i < x.size()); i++)
                    val += x.at(i);

                xi << val;

                if (inWindow < range)
                     inWindow++;
                else start++;
            }

            x = xi;
        }

        if (LoadOption == 1 || LoadOption == 3)
        {
            // scaling max to 1
            float max = -1e10;
            for (const double& d : x)
                if (d > max) max = d;
            if (max > 0)
                for (double& d : x) d /= max;
        }

        qDebug() << "After pre-processing:\n"<<x;

        PMs->at(ipm).DarkCounts_Distribution = x;
        on_pbUpdateElectronics_clicked();
    }
}

void MainWindow::on_pbDarkCounts_Delete_clicked()
{
    const int ipm = ui->sbElPMnumber->value();
    if (ipm >= PMs->count())
    {
        message("Bad PM index", this);
        return;
    }

    PMs->at(ipm).DarkCounts_Distribution.clear();
    on_pbUpdateElectronics_clicked();
}

void MainWindow::on_pushButton_clicked()
{
    QString s = "Average dark counts = ( time interval )  x  ( dark count rate of the SiPM )\n"
                "\n"
                "For time-resolved simulation the time interval is = time duration of one bin\n"
                "\n"
                "Simplistic model:\n"
                "   A dark count event is processed exactly the same way as a detected photon hit\n"
                "Advanced model:\n"
                "   A dark count event can contribute \"fractional\" number of hits\n"
                "   The contributed value is sampled from the user provided data:\n"
                "   using uniform random generator, one of the values in the provided data vector is selected."
                "   Two main load options:\n"
                "     When data loaded \"as is\", it imitates DAC which digitizes data at a given trigger."
                "     When \"integrating\" is used, it imitates DAC which integrate signal waveform in a certain range"
                "\n"
                "     Integrating range has to be smaller or equal to the provided dataset range.\n"
                "       0 values inidicates that the integration range has to be equal to the range of the provided dataset."
                "";

    message(s, this);
}

void MainWindow::on_cobDarkCounts_Model_currentIndexChanged(int index)
{
    ui->frDarkCounts_Advanced->setVisible(index != 0);
}

void MainWindow::on_cobDarkCounts_LoadOptions_currentIndexChanged(int index)
{
    ui->sbDarkCounts_IntegralRange->setEnabled(index > 1);
}

#include "awebsocketserverdialog.h"
void MainWindow::on_actionServer_window_triggered()
{
    ServerDialog->show();
}

void MainWindow::on_actionServer_settings_triggered()
{
    GlobSetWindow->SetTab(5);
    GlobSetWindow->show();
}

void MainWindow::on_actionGrid_triggered()
{
    RemoteWindow->show();
}

#include "atrackdrawdialog.h"
void MainWindow::on_pbOpenTrackProperties_Phot_clicked()
{
    QStringList pl;
    MpCollection->OnRequestListOfParticles(pl);
    ATrackDrawDialog* d = new ATrackDrawDialog(this, &SimulationManager->TrackBuildOptions, pl);
    d->exec();
    on_pbUpdateSimConfig_clicked();
}

void MainWindow::on_pbTrackOptionsGun_clicked()
{
    on_pbOpenTrackProperties_Phot_clicked();
}

void MainWindow::on_pbTrackOptionsGun_customContextMenuRequested(const QPoint &)
{   // in particle source
    SimulationManager->TrackBuildOptions.bBuildPhotonTracks = false;

    SimulationManager->TrackBuildOptions.bBuildParticleTracks = !SimulationManager->TrackBuildOptions.bBuildParticleTracks;
    on_pbUpdateSimConfig_clicked();
}

void MainWindow::on_pbOpenTrackProperties_Phot_customContextMenuRequested(const QPoint &)
{   //in photon sources
    SimulationManager->TrackBuildOptions.bBuildParticleTracks = false;

    SimulationManager->TrackBuildOptions.bBuildPhotonTracks = !SimulationManager->TrackBuildOptions.bBuildPhotonTracks;
    on_pbUpdateSimConfig_clicked();
}

void MainWindow::on_pbTrackOptionsStack_clicked()
{
    on_pbOpenTrackProperties_Phot_clicked();
}

void MainWindow::on_pbQEacceleratorWarning_clicked()
{
    QString s = "Activation of this option allows to speed up simulations\n"
            "by skipping tracking of photons which will definitely fail\n"
            "the detection test by the PMs the photon will hit.\n"
            "For this purpose the random number to be compared with the\n"
            "Quantum Efficiency of the PM is generated before\n"
            "tracking and checked against the maximum QE over all PMs.\n"
            "If detection is failed, the tracking is skipped.\n\n"
            "Important warning:\nDo not activate this feature\nif PMs can register light after wavelength shifting!";
    message(s, this);
}

void MainWindow::on_ledSphericalPMAngle_editingFinished()
{
    double val = ui->ledSphericalPMAngle->text().toDouble();
    if (val<=0 || val >180)
    {
        ui->ledSphericalPMAngle->setText("90");
        message("Angle should be a positive value not larger than 180", this);
    }

    on_pbUpdatePMproperties_clicked();
}

void MainWindow::on_lwOverrides_itemDoubleClicked(QListWidgetItem *)
{
    on_pbEditOverride_clicked();
}

void MainWindow::on_lwOverrides_itemSelectionChanged()
{
  int row = ui->lwOverrides->currentRow();

  int MatFrom = ui->cobMaterialForOverrides->currentIndex();
  int Mats = Detector->MpCollection->countMaterials();
  int counter = 0;
  for (int iMat=0; iMat<Mats; iMat++)
    {
      AOpticalOverride* ov = (*Detector->MpCollection)[MatFrom]->OpticalOverrides[iMat];
      if (!ov) continue;
      if (counter == row)
        {
          ui->cobMaterialTo->setCurrentIndex(iMat);
          return;
        }
      counter++;
    }
}

void MainWindow::on_pbShowOverrideMap_clicked()
{
    int numMat = MpCollection->countMaterials();

    QDialog* d = new QDialog(this);
    d->setWindowTitle("Map of optical overrides");
    QVBoxLayout* l = new QVBoxLayout(d);
    QLabel* lab = new QLabel("Override from (vertical) -> to (horizontal). Double click to define / edit override");
    l->addWidget(lab);
    QTableWidget* tw = new QTableWidget(numMat, numMat);
    l->addWidget(tw);

    tw->setVerticalHeaderLabels(MpCollection->getListOfMaterialNames());
    tw->setHorizontalHeaderLabels(MpCollection->getListOfMaterialNames());

    for (int ifrom = 0; ifrom < numMat; ifrom++)
        for (int ito = 0; ito < numMat; ito++)
        {
            AOpticalOverride* ov = (*MpCollection)[ifrom]->OpticalOverrides.at(ito);
            QString text;
            if (ov) text = ov->getAbbreviation();
            QTableWidgetItem *it = new QTableWidgetItem(text);
            it->setTextAlignment(Qt::AlignCenter);
            if (ov)
            {
                it->setBackground(QBrush(Qt::lightGray));
                it->setToolTip(ov->getLongReportLine());
            }
            it->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

            tw->setItem(ifrom, ito, it);
        }
    QObject::connect(tw, &QTableWidget::itemDoubleClicked, d, &QDialog::accept);

    if (bOptOvDialogPositioned)
    {
        d->resize(OptOvDialogSize);
        d->move(OptOvDialogPosition);
    }

    int res = d->exec();

    if (res == QDialog::Accepted)
    {
        int from = tw->currentRow();
        int to = tw->currentColumn();

        if (from > -1 && to > -1)
        {
            ui->cobMaterialForOverrides->setCurrentIndex(from);
            ui->cobMaterialTo->setCurrentIndex(to);
            on_pbEditOverride_clicked();
        }
    }

    bOptOvDialogPositioned = true;
    OptOvDialogSize = d->size();
    OptOvDialogPosition = d->pos();
    delete d;
}

void MainWindow::on_pbStopScan_clicked()
{
    //qDebug() << "Requesting sim stop...";
    ui->pbStopScan->setText("stopping...");
    qApp->processEvents();
    SimulationManager->StopSimulation();
    qApp->processEvents();
}

void MainWindow::on_pbPMtypeHelp_clicked()
{
    QString s = "Note that photon detection test is triggered\n"
                "   as soon as a photon eneters the PM volume!\n\n"
                "That means that the material of the PM is important\n"
                "   only for the material interface\n"
                "   (Fresnel/Snell's + optical overrides)\n\n"
                "The PM thickness matters only for positioning\n"
                "   of the volume in the detector geometry.\n\n"
                "For simulation of, e.g., PMT structure (window +\n"
                "   photocathode) one has to add the window\n"
                "   volume to the geometry and the PM object should\n"
                "   represent the photocathode.";
    message(s, this);
}

void MainWindow::on_pnShowHideAdvanced_Particle_toggled(bool checked)
{
    if (checked) ui->swAdvancedSim_Particle->setCurrentIndex(1);
            else ui->swAdvancedSim_Particle->setCurrentIndex(0);
}

void MainWindow::on_pbGoToConfigueG4ants_clicked()
{
    GlobSetWindow->showNormal();
    GlobSetWindow->SetTab(6);
}

void MainWindow::on_cbGeant4ParticleTracking_toggled(bool checked)
{
    ui->swNormalOrGeant4->setCurrentIndex((int)checked);
}

#include "ageant4configdialog.h"
void MainWindow::on_pbG4Settings_clicked()
{
    AGeant4ConfigDialog D(G4SimSet, this);
    int res = D.exec();
    if (res == 1)
    {
        //some watchdogs here?

        //update sim setgtings
        on_pbUpdateSimConfig_clicked();
    }
}

void MainWindow::on_pbSetAllInherited_clicked()
{
    PMs->resetOverrides();
    ReconstructDetector(true);
}

#include "afileparticlegenerator.h"
void MainWindow::on_pbLoadExampleFileFromFileGen_clicked()
{
    QString epff = GlobSet.ExamplesDir + "/ExampleParticlesFromFile.dat";
    SimulationManager->FileParticleGenerator->SetFileName(epff);
    updateFileParticleGeneratorGui();
}

void MainWindow::on_pbNodesFromFileHelp_clicked()
{
    QString s;
    s = "Each line in the file represents a single node.\n"
        "The format is:\n\n"
        "X Y Z [Time] [PhotNumberOverride] [*]\n\n"
        "Arguments:\n\n"
        "XYZ - position of the node\n\n"
        "Optional arguments:\n\n"
        "Time - photon generation time (0 is default)\n\n"
        "PhotNumberOverride - the provided integer value\n"
        "   overrides the standard number of photons configured for nodes.\n\n"
        "* - if present, the next node will be added to the same event.\n"
        "   '*' should be in the last position of the line.";
    message(s, this);
}

void MainWindow::on_pbNodesFromFileChange_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select a file with nodes data", GlobSet.LastOpenDir, "Data files (*.dat *.txt);;All files (*)");
    if (fileName.isEmpty()) return;
    GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
    ui->leNodesFromFile->setText(fileName);
    on_pbUpdateSimConfig_clicked();
}

#include "anoderecord.h"
void MainWindow::on_pbNodesFromFileCheckShow_clicked()
{
    QString fileName = ui->leNodesFromFile->text();
    QString err = SimulationManager->loadNodesFromFile(fileName);

    int numTop = 0;
    int numTotal = 0;
    if (!err.isEmpty()) message(err, this);
    else
    {
        Detector->GeoManager->ClearTracks();
        clearGeoMarkers();
        GeoMarkerClass* marks = new GeoMarkerClass("Nodes", 6, 2, kBlack);
        for (ANodeRecord * topNode : SimulationManager->Nodes)
        {
            numTop++;
            ANodeRecord * node = topNode;
            while (node)
            {
                numTotal++;
                if (numTotal < 10000) marks->SetNextPoint(node->getX(), node->getY(), node->getZ());
                node = node->getLinkedNode();
            }
        }
        GeoMarkers.append(marks);
        GeometryWindow->ShowGeometry();
    }

    message(QString("The file containes %1 top nodes\n(%2 nodes counting subnodes)").arg(numTop).arg(numTotal), this);
}
