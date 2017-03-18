//ANTS2 modules and windows
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "detectoraddonswindow.h"
#include "amaterialparticlecolection.h"
#include "pms.h"
#include "outputwindow.h"
#include "reconstructionwindow.h"
#include "gainevaluatorwindowclass.h"   //onPMnumChange!
#include "detectorclass.h"
#include "genericscriptwindowclass.h"
#include "globalsettingsclass.h"
#include "materialinspectorwindow.h"
#include "checkupwindowclass.h"
#include "asandwich.h"
#include "slab.h"
#include "aslablistwidget.h"
#include "ageoobject.h"
#include "ageotreewidget.h"
#include "scriptinterfaces.h"
#include "aconfiguration.h"

//Qt
#include <QDebug>
#include <QtScript>
#include <QDialog>
#include <QHBoxLayout>
#include <QMessageBox>

#include "TGeoManager.h"

void MainWindow::ReconstructDetector(bool fKeepData)
{
  //qDebug() << ">>>> ReconstructDetector (GUI method) triggered <<<<";
  int numPMs = Detector->pmCount();

  writeDetectorToJson(Config->JSON);
  Detector->BuildDetector();

  //gui update will be triggered automatically

  if (!fKeepData) MainWindow::ClearData();
  if (numPMs != Detector->pmCount())
    {
      //qDebug() << "Number of PMs changed!";
      MainWindow::NumberOfPMsHaveChanged();
    }
}

// GUI update in mainwindowjson.cpp

bool MainWindow::startupDetector()
{
  MainWindow::initDetectorSandwich(); //create detector sandwich control and link GUI signals/slots  
  if (QFile(GlobSet->ExamplesDir + "/StartupDetector.json").exists())
    {
      Config->LoadConfig(GlobSet->ExamplesDir + "/StartupDetector.json");
      return true;
    }
  else
    {
      //startup detector file not found, to avoid crash initializing make-shift detector
      qDebug() << "!!! Startup detector NOT found!";

      //dummy detector - in case startup detector json not found
      //generating a dummy detector
      Detector->Sandwich->appendSlab(new ASlabModel(true,"UpWin",   5,   0, false, 0,6,100,100,0));
      Detector->Sandwich->appendSlab(new ASlabModel(true,"PrScint", 20,   0, true,  0,6,100,100,0));

      //Generate MaterialCollection
      MpCollection->AddNewMaterial();
      MpCollection->UpdateMaterial(MpCollection->countMaterials()-1, "Air", 1.2041e-3, 0, 1, 0, 5, 100, 2, 5, 1, 0, 0, 0);
      AddDefaultPMtype();
      MainWindow::on_pbRefreshMaterials_clicked();
      MainWindow::on_pbRefreshOverrides_clicked();
      //create detector geometry and visualize
      qDebug()<<"-> Pre-building make-shift detector";
      Detector->PMarrays[0].fActive = ui->cbUPM->isChecked();
      Detector->PMarrays[1].fActive = ui->cbLPM->isChecked();
      Detector->PMarrays[0].fUseRings = true;
      Detector->PMarrays[0].CenterToCenter = 30;

      Detector->GeoManager = new TGeoManager();
      Detector->Sandwich->UpdateDetector();

      MIwindow->SetMaterial(0);
      MIwindow->SetParticleSelection(0);

      MainWindow::on_pbShowPMsArrayRegularData_clicked();
      return false;
    }
}

void MainWindow::initDetectorSandwich()
{
  //ListWidget for slabs
  lw = new ASlabListWidget(Detector->Sandwich);
  QVBoxLayout* laySandwich = new QVBoxLayout();
    laySandwich->setContentsMargins(0,0,0,0);
    ui->saSandwich->setLayout(laySandwich);
    connect(Detector->Sandwich, SIGNAL(RequestGuiUpdate()), lw, SLOT(UpdateGui()));
    lw->UpdateGui();
  laySandwich->addWidget(lw);
  connect(lw, SIGNAL(RequestHighlightObject(QString)), DAwindow, SLOT(ShowObject(QString)));

  //Default XY properties
  ASlabXYDelegate* DefaultXY_delegate = lw->GetDefaultXYdelegate();
  QHBoxLayout* xyl = new QHBoxLayout();
    xyl->setContentsMargins(0,0,0,0);
    xyl->addWidget(DefaultXY_delegate);
  ui->fCommonXY->setLayout(xyl);

  MainWindow::UpdateSandwichGui();

  connect(Detector->Sandwich, SIGNAL(WarningMessage(QString)), this, SLOT(OnWarningMessage(QString)));
  connect(Detector->Sandwich, SIGNAL(RequestGuiUpdate()), this, SLOT(UpdateSandwichGui()));
  connect(Detector->Sandwich, SIGNAL(RequestRebuildDetector()), this, SLOT(on_pbRebuildDetector_clicked()));
  connect(Detector, SIGNAL(ColorSchemeChanged(int,int)), this, SLOT(OnDetectorColorSchemeChanged(int,int)));
  connect(lw, SIGNAL(SlabDoubleClicked(QString)), this, SLOT(OnSlabDoubleClicked(QString)));

  QString help = "Basic detector consists of a stack of slabs.\n"
      "Choose here whether all slabs have the same shape and size,\n"
      "  have the same shape but allowed to have different size,\n"
      "  or can have individual shape (and size).\n\n"
      "Slab properties can be modified right clicking on the corresponding slab!\n"
      "New slabs can be created also using the right-mouse-click menu.";
  ui->cobXYtype->setToolTip(help);
  ui->label_285->setToolTip(help);  
}

void MainWindow::OnSlabDoubleClicked(QString SlabName)
{
  DAwindow->show();
  DAwindow->raise();
  DAwindow->activateWindow();

  DAwindow->SetTab(0);

  QStringList selection;
  selection << SlabName;
  DAwindow->twGeo->SelectObjects(selection);
}

void MainWindow::UpdateSandwichGui()
{
  //qDebug() << "Update Sandwich GUI requested";

  switch (Detector->Sandwich->SandwichState)
    {
    case ASandwich::CommonShapeSize:
      ui->cobXYtype->setCurrentIndex(0);
      ui->saSandwich->resize(388, ui->saSandwich->height());
      ui->fCommonXY->setVisible(true);
      break;
    case ASandwich::CommonShape:
      ui->cobXYtype->setCurrentIndex(1);
      ui->saSandwich->resize(388, ui->saSandwich->height());
      ui->fCommonXY->setVisible(true);
      break;
    case ASandwich::Individual:
      ui->cobXYtype->setCurrentIndex(2);
      ui->saSandwich->resize(528, ui->saSandwich->height());
      ui->fCommonXY->setVisible(false);
      break;
    }

  ui->cobTOP->clear();
  ui->cobTOP->addItems(Detector->Sandwich->GetMaterials());
  int iWM = Detector->Sandwich->World->Material;
  if (iWM<0 || iWM>ui->cobTOP->count()-1) ui->cobTOP->setCurrentIndex(-1);
  else ui->cobTOP->setCurrentIndex(iWM);  
}

void MainWindow::on_cobTOP_activated(int index)
{
  Detector->Sandwich->World->Material = index;
  ReconstructDetector(true);
}

void MainWindow::OnWarningMessage(QString text)
{
  QMessageBox::warning(this, "Warning!", text);
}

void MainWindow::OnDetectorColorSchemeChanged(int scheme, int /*matId*/)
{
  //Slabs
  int slabIndex = -1;
  for (int iHO=0; iHO<Detector->Sandwich->World->HostedObjects.size(); iHO++)
    {
      const AGeoObject* obj = Detector->Sandwich->World->HostedObjects.at(iHO);
      if (!obj->ObjectType->isSlab()) continue;
      slabIndex++; //starts with 0

      ASlabDelegate* ld = static_cast<ASlabDelegate*>(lw->itemWidget(lw->item(slabIndex)));
      if (scheme == 0)
        {
          int color = obj->color;
          if (color != -1) ld->SetColorIndicator(color);
          ld->frColor->setVisible(true);
        }
      else
        {
          ld->frColor->setVisible(false);
        }
    }  
}

void MainWindow::CheckPresenseOfSecScintillator()
{   
  Detector->checkSecScintPresent();

  if (!Detector->fSecScintPresent)
    {
      ui->cobScintTypePointSource->setCurrentIndex(0);
      ui->cbGunDoS2->setChecked(false);
      ui->cbDoS2tester->setChecked(false);
    }
  ui->cobScintTypePointSource->setEnabled(Detector->fSecScintPresent);
  ui->cbGunDoS2->setEnabled(Detector->fSecScintPresent);
  ui->cbDoS2tester->setEnabled(Detector->fSecScintPresent);
}

void MainWindow::NumberOfPMsHaveChanged()
{  
  int NumPMs = PMs->count();

  //this window
  if (ui->sbIndPMnumber->value() > NumPMs-1) ui->sbIndPMnumber->setValue(0);
  if (ui->sbElPMnumber->value() > NumPMs-1) ui->sbElPMnumber->setValue(0);
  if (ui->sbPreprocessigPMnumber->value() > NumPMs-1) ui->sbPreprocessigPMnumber->setValue(0);

  if (Rwindow)
    {
      Rwindow->PMnumChanged();
      //Rwindow->UpdatePMgroupIndication();
    }
  if (Owindow)
   {    
     Owindow->RefreshData();
     Owindow->PMnumChanged();
   }
  if (GainWindow) GainWindow->Reset();

  ui->sbLoadedEnergyChannelNumber->setValue(NumPMs);  
  ui->fReloadRequired->setVisible(false); 
}

void MainWindow::on_pbPositionScript_clicked()
{
  extractGeometryOfScriptWindow();
  if (GenScriptWindow) delete GenScriptWindow;
  GenScriptWindow = new GenericScriptWindowClass(Detector->RandGen);
  recallGeometryOfScriptWindow();
  int ul = ui->cobUpperLowerPMs->currentIndex();

  //configure the script window and engine
  PMscriptInterface = new InterfaceToPMscript(); //deleted by the GenScriptWindow
  GenScriptWindow->SetInterfaceObject(PMscriptInterface);

  GenScriptWindow->SetShowEvaluationResult(false); //do not show "undefined"
  GenScriptWindow->SetExample("for (var i=0; i<3; i++) PM(i*60, (i-2)*60, 0, 0)");

  QString s = (ul==0) ? "upper array":"lower array";
  GenScriptWindow->SetTitle("Position PMs: "+s);

  GenScriptWindow->SetScript(&Detector->PMarrays[ul].PositioningScript);  

  GenScriptWindow->SetStarterDir(GlobSet->LibScripts);

  //define what to do on evaluation success
  connect(GenScriptWindow, SIGNAL(success(QString)), this, SLOT(PMscriptSuccess()));
  //if needed. connect signals of the interface object with the required slots of any ANTS2 objects
  GenScriptWindow->show();  
}

void MainWindow::PMscriptSuccess()
{
  QJsonArray &arr = PMscriptInterface->arr;
  if (arr.isEmpty())
    {
      GenScriptWindow->ShowText("No PMs were added!");
      return;
    }
  int ul = ui->cobUpperLowerPMs->currentIndex();
  APmArrayData *PMarr = &Detector->PMarrays[ul];
  int reg = PMarr->Regularity;
  //checking data
  for (int ipm=0; ipm<arr.size(); ipm++)
    {
      QJsonArray el = arr[ipm].toArray();
      int size = el.size();
      if (reg == 2)
        {
          if (size < 4)
            {
              GenScriptWindow->ReportError("Cannot use 'pm' command with fully custom arrays - need Z coordinate!");
              return;
            }
        }
      if (el.size()>3)
        {
          QString type = el[3].toString();
          //qDebug() << el[3] << el[3].toString();
          int PMtype = Detector->PMs->findPMtype(type);
          if (PMtype == -1)
            {
              bool ok = false;
              PMtype = type.toInt(&ok);
              if (!ok)
                {
                  GenScriptWindow->ReportError("PM type should be either the name or the index");
                  return;
                }
            }
          if (PMtype<0 || PMtype > Detector->PMs->countPMtypes()-1)
            {
              GenScriptWindow->ReportError("Non-existent PM type!");
              return;
            }
        }
      if (el.size()>4)
        {
           double phi = el[4].toDouble();
           double theta = el[5].toDouble();
           if (reg == 1 && (phi!=0 || theta!=0) )
             {
               //GenScriptWindow->ShowText("Cannot assign phi or theta angles - change to fully custom array geometry!");
               //return;
               qDebug() << "Changing regularity to fully custom";
               GenScriptWindow->ShowText("Changed array regularity to fully custom!");
               PMarr->Regularity = 2;
               reg = 2;
             }
        }
    }
  //copy data
  PMarr->PositionsAnglesTypes.clear();
  for (int ipm=0; ipm<arr.size(); ipm++)
      {
        QJsonArray el = arr[ipm].toArray();
        int size = el.size();
        //qDebug() << "Element to process:"<<el;
        double x = el[0].toDouble();
        double y = el[1].toDouble();
        if (size < 3)
          {
            PMarr->PositionsAnglesTypes.append(APmPosAngTypeRecord(x, y, 0, 0,0,0, 0));
            continue;
          }
        double z=0, phi=0, theta=0, psi=0;
        int type=0;
        if (size>2)
          {
            z = el[2].toDouble();
            QString Stype = el[3].toString();
            type = Detector->PMs->findPMtype(Stype);
            if (type == -1) type = Stype.toInt();
          }
        if (size>4)
          {
            phi = el[4].toDouble();
            theta = el[5].toDouble();
            psi = el[6].toDouble();
          }
        PMarr->PositionsAnglesTypes.append(APmPosAngTypeRecord(x, y, z, phi,theta,psi, type));
      }
  //clean array in case the script will be called again
  PMscriptInterface->arr = QJsonArray();
  ReconstructDetector();
  updatePMArrayDataIndication();

  int nooverlaps = CheckUpWindow->CheckGeoOverlaps();
  if (nooverlaps != 0) CheckUpWindow->show();
}

void MainWindow::on_cobXYtype_activated(int index)
{
  ASandwich::SlabState state = (ASandwich::SlabState)index;
  Detector->Sandwich->ChangeState(state);
}
