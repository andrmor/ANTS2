#include "reconstructionwindow.h"
#include "ui_reconstructionwindow.h"
#include "mainwindow.h"
#include "guiutils.h"
#include "detectorclass.h"
#include "apmgroupsmanager.h"
#include "CorrelationFilters.h"
#include "alrfmoduleselector.h"
//#include "arepository.h"
#include "areconstructionmanager.h"
#include "acalibratorsignalperphel.h"

#ifdef ANTS_FLANN
#include "areconstructionmanager.h"
#include "nnmoduleclass.h"
#endif

#include <QDebug>

#include "TLine.h"
#include "TEllipse.h"
#include "TPolyLine.h"

ReconstructionWindow::ReconstructionWindow(QWidget * parent, MainWindow * mw, EventsDataClass * eventsDataHub) :
  AGuiWindow("recon", parent),
  MW(mw),
  ui(new Ui::ReconstructionWindow),
  ReconstructionManager(MW->ReconstructionManager),
  EventsDataHub(eventsDataHub),
  Detector(MW->Detector),
  PMgroups(MW->Detector->PMgroups),
  PMs(MW->PMs)
{
  CorrelationUnitGenericClass* X = new CU_SingleChannel({0}, eventsDataHub, PMgroups, PMgroups->getCurrentGroup());
  CorrelationUnitGenericClass* Y = new CU_SingleChannel({0}, eventsDataHub, PMgroups, PMgroups->getCurrentGroup());
  CorrelationCutGenericClass* Cut = new Cut_Line();
  Cut->Data << 0 << 1 << 300;
  tmpCorrFilter = new CorrelationFilterStructure(X, Y, Cut);
  tmpCorrFilter->Active = true;

  ui->setupUi(this);
  this->setFixedSize(this->size());

  Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
  windowFlags |= Qt::WindowCloseButtonHint;
  //windowFlags |= Qt::Tool;
  this->setWindowFlags( windowFlags );

  CorrCutLine    = new TLine(0,0,0,0); //invisible
  CorrCutEllipse = new TEllipse(0, 0, 0, 0, 0, 360, 0); //invisible
  CorrCutPolygon = new TPolyLine(0);

  funcParams[0] = 0;
  funcParams[1] = 0;
  funcParams[2] = 1.0;
  funcParams[3] = 1.0;

  RedIcon = GuiUtils::createColorCircleIcon(ui->twOptions->iconSize(), Qt::red);
  ui->labDataIsMissing->setPixmap(RedIcon.pixmap(16,16));
  ui->fNoData->setVisible(true);
  YellowIcon = GuiUtils::createColorCircleIcon(ui->twData->iconSize(), Qt::yellow);
  ui->labUnassignedIcon->setPixmap(YellowIcon.pixmap(16,16));
  ui->fNotAssignedPMs->setVisible(false);

  QString style = "QLabel { background-color: #E0E0E0; }";  //setting background color for the "Configuration" tab
  ui->laBackground->setStyleSheet(style);
  ui->twData->setCurrentIndex(0);
  ui->twOptions->setCurrentIndex(0);
  ui->cbRecType->setCurrentIndex(-1); //frozen development: point or track reconstruction
  ui->cbRecType->setCurrentIndex(0);

  QList<QWidget*> invis;
  invis << ui->pbUpdateFilters << ui->pbSpF_UpdateTable << ui->pbUpdateGainsIndication << ui->pbCorrUpdateTMP
        << ui->fShowXYPmnumber << ui->pbKNNupdate << ui->pbUpdateReconConfig << ui->labLRFmoduleWarning << ui->pbUpdateGuiSettingsInJSON
        << ui->pbRootConfigureCustom;
  for (auto w: invis) w->setVisible(false);

  QList<QWidget*> disab;
  disab << ui->leoTimeBins // ui->fFilterSumSignal << ui->fFilterIndividualSignal << ui->fEnergyFilter << ui->fChi2Filter << ui->fLoadedEnergyFilter
        << ui->labTimeBins << ui->fCorrCut << ui->fCorrAddRemoe << ui->fCorrSize << ui->cbCorrShowCut << ui->dLoadedEnergy
        << ui->cbForceCoGgiveZof << ui->fCustomBins << ui->fCustomRanges << ui->fInRecFilter << ui->fDynPassive;
  for (auto w: disab) w->setEnabled(false);
  ui->fCustomSpatFilter->setEnabled(ui->cbSpFcustom->isChecked());
  ui->fSpFz->setEnabled(!ui->cbSpFallZ->isChecked());

  QPalette palette = ui->leSensorGroupComposition->palette();
  palette.setColor(QPalette::Base, "#F0F0F0");
  ui->leSensorGroupComposition->setPalette(palette);

#ifdef ANTS_FLANN
  ui->fNNfilter->setVisible(true);
  ui->labFLANNenabled->setVisible(false);

  connect(&MW->ReconstructionManager->KNNmodule->Reconstructor, SIGNAL(readyXchanged(bool, int)), this, SLOT(onKNNreadyXchanged(bool, int)));
  connect(&MW->ReconstructionManager->KNNmodule->Reconstructor, SIGNAL(readyYchanged(bool, int)), this, SLOT(onKNNreadyYchanged(bool, int)));
#else
  ui->fNNfilter->setVisible(false);
  ui->labFLANNenabled->setVisible(true);
#endif

  QDoubleValidator* dv = new QDoubleValidator(this);
  dv->setNotation(QDoubleValidator::ScientificNotation);
  QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
  foreach(QLineEdit *w, list) if (w->objectName().startsWith("led")) w->setValidator(dv);

  QIntValidator* iv  = new QIntValidator(this);
  iv->setBottom(0);  
  foreach(QLineEdit *w, list) if (w->objectName().startsWith("lei")) w->setValidator(iv);

  ui->cbDynamicPassiveByDistance->setChecked(false);
  ui->cbDynamicPassiveBySignal->setChecked(false);

  ui->twOptions->tabBar()->setTabIcon(2, RedIcon); //assuming there are no LRF on start

  ui->tabwidCorrPolygon->resizeColumnsToContents();
  ui->tabwidCorrPolygon->resizeRowsToContents();

  ui->tabWidget->setCurrentIndex(1); //reconstruction options tab
  ui->bsAnalyzeScan->setCurrentIndex(0); //reconstruct all tab
  ui->cobCUDAoffsetOption->setCurrentIndex(1);
  ui->cobCUDAoffsetOption->setCurrentIndex(0);

  QObject::connect(Detector, SIGNAL(requestGroupsGuiUpdate()), this, SLOT(onUpdatePMgroupIndication()));

  //connections - the slot function is aware of the currently active module and handles signal distribution to MW
  //old module:
  const SensorLRFs* SensLRF = Detector->LRFs->getOldModule();
  QObject::connect(SensLRF, &SensorLRFs::SensorLRFsReadySignal, this, &ReconstructionWindow::LRF_ModuleReadySlot);
  //new module:
  //const LRF::ARepository* NewModule = Detector->LRFs->getNewModule();
  //QObject::connect(NewModule, &LRF::ARepository::currentLrfsChangedReadyStatus, this, &ReconstructionWindow::LRF_ModuleReadySlot);

  ui->cobLRFmodule->setCurrentIndex(0);
  on_cobLRFmodule_currentIndexChanged(0);

  //handling of NN module reconstruction options
  connect(ui->cbReconstructEnergy,SIGNAL(clicked(bool)),this,SIGNAL(cbReconstructEnergyChanged(bool)));
  connect(ui->cb3Dreconstruction,SIGNAL(clicked(bool)),this,SIGNAL(cb3DreconstructionChanged(bool)));

  on_cobReconstructionAlgorithm_currentIndexChanged(ui->cobReconstructionAlgorithm->currentIndex());
  on_cobCGstartOption_currentIndexChanged(ui->cobCGstartOption->currentIndex());

  on_cbLimitNumberEvents_toggled(ui->cbLimitNumberEvents->isChecked());
  on_cobHowToAverageZ_currentIndexChanged(ui->cobHowToAverageZ->currentIndex());

  connect(ReconstructionManager->Calibrator_Stat,  &ACalibratorSignalPerPhEl_Stat::progressChanged, this, &ReconstructionWindow::SetProgress);
  connect(ReconstructionManager->Calibrator_Peaks, &ACalibratorSignalPerPhEl_Stat::progressChanged, this, &ReconstructionWindow::SetProgress);

   // qDebug()<<"  Reconstruction Window created";
}

