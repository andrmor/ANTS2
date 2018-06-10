#include "globalsettingswindowclass.h"
#include "ui_globalsettingswindowclass.h"
#include "mainwindow.h"
#include "ajavascriptmanager.h"
#include "ascriptwindow.h"
#include "globalsettingsclass.h"
#include "detectorclass.h"
#include "amessage.h"
#include "anetworkmodule.h"
#include "geometrywindowclass.h"
#include "ainterfacetogstylescript.h"

//Qt
#include <QFileDialog>
#include <QDesktopServices>
#include <QDebug>

#include "TGeoManager.h"

GlobalSettingsWindowClass::GlobalSettingsWindowClass(MainWindow *parent) :
  QMainWindow(parent),
  ui(new Ui::GlobalSettingsWindowClass)
{
  MW = parent;  
  GlobSet = MW->GlobSet;
  ui->setupUi(this);

  Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
  windowFlags |= Qt::WindowCloseButtonHint;
  windowFlags |= Qt::WindowStaysOnTopHint;
  this->setWindowFlags( windowFlags );

  updateGUI();
  QObject::connect(GlobSet->NetModule, &ANetworkModule::StatusChanged, this, &GlobalSettingsWindowClass::updateNetGui);

  if (GlobSet->fRunRootServerOnStart)
    GlobSet->NetModule->StartRootHttpServer(GlobSet->DefaultRootServerPort, GlobSet->ExternalJSROOT);  //does nothing if compilation flag is not set
  if (GlobSet->fRunWebSocketServerOnStart)
    GlobSet->NetModule->StartWebSocketServer(GlobSet->DefaultWebSocketPort);
}

GlobalSettingsWindowClass::~GlobalSettingsWindowClass()
{
   delete ui; ui = 0;
}

void GlobalSettingsWindowClass::updateGUI()
{
  ui->leLibPMtypes->setText(GlobSet->LibPMtypes);
  ui->leLibMaterials->setText(GlobSet->LibMaterials);
  ui->leLibParticleSources->setText(GlobSet->LibParticleSources);
  ui->leLibScripts->setText(GlobSet->LibScripts);

  ui->leWorkingDir->setText(GlobSet->TmpDir);

  ui->sbFontSize->setValue(GlobSet->FontSize);

  //ui->rbNever->setChecked(GlobSet->NeverSaveOnExit);
  //ui->rbAlways->setChecked(GlobSet->AlwaysSaveOnExit);
  //ui->rbAskMe->setChecked(!GlobSet->NeverSaveOnExit && !GlobSet->AlwaysSaveOnExit);

  ui->cbOpenImageExternalEditor->setChecked(GlobSet->fOpenImageExternalEditor);

  ui->sbLogPrecision->setValue(GlobSet->TextLogPrecision);

  ui->sbNumBinsHistogramsX->setValue(GlobSet->BinsX);
  ui->sbNumBinsHistogramsY->setValue(GlobSet->BinsY);
  ui->sbNumBinsHistogramsZ->setValue(GlobSet->BinsZ);

  ui->sbNumPointsFunctionX->setValue(GlobSet->FunctionPointsX);
  ui->sbNumPointsFunctionY->setValue(GlobSet->FunctionPointsY);

  ui->sbNumSegments->setValue(GlobSet->NumSegments);
  ui->sbMaxNumTracks->setValue(GlobSet->MaxNumberOfTracks);

  ui->sbNumTreads->setValue(GlobSet->NumThreads);
  ui->sbRecNumTreads->setValue(GlobSet->RecNumTreads);

  ui->cbSaveRecAsTree_IncludePMsignals->setChecked(GlobSet->RecTreeSave_IncludePMsignals);
  ui->cbSaveRecAsTree_IncludeRho->setChecked(GlobSet->RecTreeSave_IncludeRho);
  ui->cbSaveRecAsTree_IncludeTrue->setChecked(GlobSet->RecTreeSave_IncludeTrue);

  updateNetGui();
}

void GlobalSettingsWindowClass::updateNetGui()
{
  bool fWebSocketRunning = GlobSet->NetModule->isWebSocketServerRunning();
  ui->cbRunWebSocketServer->setChecked( fWebSocketRunning );
  ui->cbAutoRunWebSocketServer->setChecked( GlobSet->fRunWebSocketServerOnStart );
  if (fWebSocketRunning)
    {
      int port = GlobSet->NetModule->getWebSocketPort();
      ui->leWebSocketPort->setText(QString::number(port));
      ui->leWebSocketURL->setText(GlobSet->NetModule->getWebSocketServerURL());
    }

  bool fRootServerRunning = GlobSet->NetModule->isRootServerRunning();
  ui->cbRunRootServer->setChecked( fRootServerRunning );
  ui->cbAutoRunRootServer->setChecked( GlobSet->fRunRootServerOnStart );

#ifdef USE_ROOT_HTML
  if (fRootServerRunning)
    {
      ui->leJSROOT->setText( GlobSet->NetModule->getJSROOTstring());
      int port = GlobSet->NetModule->getRootServerPort();
      QString sPort = QString::number(port);
      ui->leRootServerPort->setText(sPort);
      QString url = "http://localhost:" + sPort;
      ui->leRootServerURL->setText(url);
    }
#else
  ui->cbRunRootServer->setChecked(false);
  ui->cbRunRootServer->setEnabled(false);
  ui->cbAutoRunRootServer->setEnabled(false);
  ui->leRootServerPort->setEnabled(false);
  ui->leJSROOT->setEnabled(false);
  ui->leRootServerURL->setEnabled(false);
#endif
}

void GlobalSettingsWindowClass::SetTab(int iTab)
{
    if (iTab<0 || iTab>ui->twMain->count()-1) return;
    ui->twMain->setCurrentIndex(iTab);
}

void GlobalSettingsWindowClass::on_pbgStyleScript_clicked()
{
    MW->extractGeometryOfLocalScriptWindow();
    delete MW->GenScriptWindow; MW->GenScriptWindow = 0;

    AJavaScriptManager* jsm = new AJavaScriptManager(MW->Detector->RandGen);
    MW->GenScriptWindow = new AScriptWindow(jsm, GlobSet, true, this);

    QString example = QString("");
    MW->GenScriptWindow->ConfigureForLightMode(&GlobSet->RootStyleScript,
                              "Script to set ROOT's gStyle",
                              example);

    GStyleInterface = new AInterfaceToGStyleScript();
    MW->GenScriptWindow->SetInterfaceObject(GStyleInterface);

    MW->recallGeometryOfLocalScriptWindow();
    MW->GenScriptWindow->show();

  /*
  MW->extractGeometryOfLocalScriptWindow();
  if (MW->GenScriptWindow) delete MW->GenScriptWindow;
  MW->GenScriptWindow = new GenericScriptWindowClass(MW->Detector->RandGen);
  MW->recallGeometryOfLocalScriptWindow();

  //configure the script window and engine
  GStyleInterface  = new  InterfaceToGStyleScript() ; //deleted by the GenScriptWindow
  MW->GenScriptWindow->SetInterfaceObject(GStyleInterface);  
  //QString HelpText = "  Avalable commands: \nsee https://root.cern.ch/root/html/TStyle.html - all commands starting from \"Set\"\n";
  MW->GenScriptWindow->SetExample("");
  MW->GenScriptWindow->SetShowEvaluationResult(false); //do not show "undefined"
  MW->GenScriptWindow->SetTitle("Script to set ROOT's gStyle");
  MW->GenScriptWindow->SetScript(&GlobSet->RootStyleScript);
  MW->GenScriptWindow->SetStarterDir(MW->GlobSet->LibScripts);

  //define what to do on evaluation success
  //connect(MW->GenScriptWindow, SIGNAL(success(QString)), this, SLOT(HolesScriptSuccess()));
  //if needed. connect signals of the interface object with the required slots of any ANTS2 objects
  MW->GenScriptWindow->show();
  */
}

void GlobalSettingsWindowClass::on_pbChoosePMtypeLib_clicked()
{
  QString starter = (GlobSet->LibPMtypes.isEmpty()) ? GlobSet->LastOpenDir : GlobSet->LibPMtypes;
  QString dir = QFileDialog::getExistingDirectory(this, "Select directory for PM types",starter,QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (dir.isEmpty()) return;
  GlobSet->LibPMtypes = dir;
  ui->leLibPMtypes->setText(dir);
}
void GlobalSettingsWindowClass::on_leLibPMtypes_editingFinished()
{
  GlobSet->LibPMtypes = ui->leLibPMtypes->text();
}

void GlobalSettingsWindowClass::on_pbChooseMaterialLib_clicked()
{
  QString starter = (GlobSet->LibMaterials.isEmpty()) ? GlobSet->LastOpenDir : GlobSet->LibMaterials;
  QString dir = QFileDialog::getExistingDirectory(this, "Select directory for materials",starter,QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (dir.isEmpty()) return;
  GlobSet->LibMaterials = dir;
  ui->leLibMaterials->setText(dir);
}
void GlobalSettingsWindowClass::on_leLibMaterials_editingFinished()
{
  GlobSet->LibMaterials = ui->leLibMaterials->text();
}

void GlobalSettingsWindowClass::on_pbChooseParticleSourcesLib_clicked()
{
  QString starter = (GlobSet->LibParticleSources.isEmpty()) ? GlobSet->LastOpenDir : GlobSet->LibParticleSources;
  QString dir = QFileDialog::getExistingDirectory(this, "Select directory for particle sources",starter,QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (dir.isEmpty()) return;
  GlobSet->LibParticleSources = dir;
  ui->leLibParticleSources->setText(dir);
}
void GlobalSettingsWindowClass::on_leLibParticleSources_editingFinished()
{
  GlobSet->LibParticleSources = ui->leLibParticleSources->text();
}

void GlobalSettingsWindowClass::on_pbChooseScriptsLib_clicked()
{
  QString starter = (GlobSet->LibScripts.isEmpty()) ? GlobSet->LastOpenDir : GlobSet->LibScripts;
  QString dir = QFileDialog::getExistingDirectory(this, "Select directory for scripts",starter,QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (dir.isEmpty()) return;
  GlobSet->LibScripts = dir;
  ui->leLibScripts->setText(dir);
}
void GlobalSettingsWindowClass::on_leLibScripts_editingFinished()
{
  GlobSet->LibScripts = ui->leLibScripts->text();
}

void GlobalSettingsWindowClass::on_pbChangeWorkingDir_clicked()
{
  QString starter = GlobSet->TmpDir;
  QString dir = QFileDialog::getExistingDirectory(this, "Select working directory (temporary files are saved here)",starter,QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (dir.isEmpty()) return;
  GlobSet->TmpDir = dir;
  QDir::setCurrent(GlobSet->TmpDir);
  ui->leWorkingDir->setText(dir);
}
void GlobalSettingsWindowClass::on_leWorkingDir_editingFinished()
{
   QString dir = ui->leWorkingDir->text();
   if ( QDir(dir).exists() )
     {
       GlobSet->TmpDir = dir;
       QDir::setCurrent(GlobSet->TmpDir);
       ui->leWorkingDir->setText(dir);
     }
   else
     {
       ui->leWorkingDir->setText(GlobSet->TmpDir);
       message("Dir does not exists!", this);
     }
}

void GlobalSettingsWindowClass::on_sbFontSize_editingFinished()
{
    GlobSet->FontSize = ui->sbFontSize->value();
    MW->setFontSizeAllWindows(GlobSet->FontSize);
}

void GlobalSettingsWindowClass::on_rbAlways_toggled(bool /*checked*/)
{
  //GlobSet->AlwaysSaveOnExit = checked;
}

void GlobalSettingsWindowClass::on_rbNever_toggled(bool /*checked*/)
{
  //GlobSet->NeverSaveOnExit = checked;
}

void GlobalSettingsWindowClass::on_cbOpenImageExternalEditor_clicked(bool checked)
{
  GlobSet->fOpenImageExternalEditor = checked;
}

void GlobalSettingsWindowClass::on_sbLogPrecision_editingFinished()
{
  GlobSet->TextLogPrecision = ui->sbLogPrecision->value();
}

void GlobalSettingsWindowClass::on_sbNumBinsHistogramsX_editingFinished()
{
  GlobSet->BinsX = ui->sbNumBinsHistogramsX->value();
}

void GlobalSettingsWindowClass::on_sbNumBinsHistogramsY_editingFinished()
{
  GlobSet->BinsY = ui->sbNumBinsHistogramsY->value();
}

void GlobalSettingsWindowClass::on_sbNumBinsHistogramsZ_editingFinished()
{
  GlobSet->BinsZ = ui->sbNumBinsHistogramsZ->value();
}

void GlobalSettingsWindowClass::on_pbChoosePMtypeLib_customContextMenuRequested(const QPoint &/*pos*/)
{
  if (ui->leLibPMtypes->text().isEmpty()) return;
  QDesktopServices::openUrl(QUrl("file///:"+ui->leLibPMtypes->text(), QUrl::TolerantMode));
}

void GlobalSettingsWindowClass::on_pbChooseMaterialLib_customContextMenuRequested(const QPoint &/*pos*/)
{
  if (ui->leLibMaterials->text().isEmpty()) return;
  QDesktopServices::openUrl(QUrl("file:///"+ui->leLibMaterials->text(), QUrl::TolerantMode));
}

void GlobalSettingsWindowClass::on_pbChooseParticleSourcesLib_customContextMenuRequested(const QPoint &/*pos*/)
{
  if (ui->leLibParticleSources->text().isEmpty()) return;
  QDesktopServices::openUrl(QUrl("file:///"+ui->leLibParticleSources->text(), QUrl::TolerantMode));
}

void GlobalSettingsWindowClass::on_pbChooseScriptsLib_customContextMenuRequested(const QPoint &/*pos*/)
{
  if (ui->leLibScripts->text().isEmpty()) return;
  QDesktopServices::openUrl(QUrl("file:///"+ui->leLibScripts->text(), QUrl::TolerantMode));
}

void GlobalSettingsWindowClass::on_pbOpen_clicked()
{
  QString what;
  switch (ui->comboBox->currentIndex())
    {
    case 0:
      what = GlobSet->AntsBaseDir;
      break;
    case 1:
      what = GlobSet->ConfigDir;
      break;
    case 2:
      what = GlobSet->LibScripts;
      break;
    case 3:
      what = GlobSet->LastOpenDir;
      break;
    case 4:
      what = GlobSet->ExamplesDir;
      break;
    }
  QDesktopServices::openUrl(QUrl("file:///"+what, QUrl::TolerantMode));
}

void GlobalSettingsWindowClass::on_sbNumSegments_editingFinished()
{
    GlobSet->NumSegments = ui->sbNumSegments->value();
    MW->Detector->GeoManager->SetNsegments(GlobSet->NumSegments);
    MW->GeometryWindow->ShowGeometry(false);
}

void GlobalSettingsWindowClass::on_sbMaxNumTracks_editingFinished()
{
   GlobSet->MaxNumberOfTracks = ui->sbMaxNumTracks->value();
}

void GlobalSettingsWindowClass::on_sbNumPointsFunctionX_editingFinished()
{
   GlobSet->FunctionPointsX = ui->sbNumPointsFunctionX->value();
}

void GlobalSettingsWindowClass::on_sbNumPointsFunctionY_editingFinished()
{
   GlobSet->FunctionPointsY = ui->sbNumPointsFunctionY->value();
}

void GlobalSettingsWindowClass::on_sbNumTreads_valueChanged(int arg1)
{
    ui->fMultiDisabled->setVisible(arg1 == 0);
}

void GlobalSettingsWindowClass::on_sbNumTreads_editingFinished()
{
    GlobSet->NumThreads = ui->sbNumTreads->value();
}

void GlobalSettingsWindowClass::on_cbSaveRecAsTree_IncludePMsignals_clicked(bool checked)
{
    GlobSet->RecTreeSave_IncludePMsignals = checked;
}

void GlobalSettingsWindowClass::on_cbSaveRecAsTree_IncludeRho_clicked(bool checked)
{
    GlobSet->RecTreeSave_IncludeRho = checked;
}

void GlobalSettingsWindowClass::on_cbSaveRecAsTree_IncludeTrue_clicked(bool checked)
{
    GlobSet->RecTreeSave_IncludeTrue = checked;
}

void GlobalSettingsWindowClass::on_cbRunWebSocketServer_clicked(bool checked)
{
  if (checked)
    GlobSet->NetModule->StartWebSocketServer(GlobSet->DefaultWebSocketPort);
  else
    GlobSet->NetModule->StopWebSocketServer();
}

void GlobalSettingsWindowClass::on_cbAutoRunWebSocketServer_clicked()
{
  GlobSet->fRunWebSocketServerOnStart = ui->cbAutoRunWebSocketServer->isChecked();
}

void GlobalSettingsWindowClass::on_leWebSocketPort_editingFinished()
{
  int oldp = GlobSet->DefaultWebSocketPort;
  int newp = ui->leWebSocketPort->text().toInt();
  if (oldp == newp) return;

  GlobSet->DefaultWebSocketPort = newp;
  if (ui->cbRunWebSocketServer->isChecked())
      GlobSet->NetModule->StartWebSocketServer(GlobSet->DefaultWebSocketPort);
}

void GlobalSettingsWindowClass::on_cbRunRootServer_clicked(bool checked)
{
    if (checked)
      GlobSet->NetModule->StartRootHttpServer(GlobSet->DefaultRootServerPort, GlobSet->ExternalJSROOT);  //does nothing if compilation flag is not set
    else
      GlobSet->NetModule->StopRootHttpServer();
}

void GlobalSettingsWindowClass::on_cbAutoRunRootServer_clicked()
{
  GlobSet->fRunRootServerOnStart = ui->cbAutoRunRootServer->isChecked();
}

void GlobalSettingsWindowClass::on_leRootServerPort_editingFinished()
{
  int oldp = GlobSet->DefaultWebSocketPort;
  int newp = ui->leRootServerPort->text().toInt();
  if (oldp == newp) return;
  GlobSet->DefaultRootServerPort = ui->leRootServerPort->text().toInt();

  if (ui->cbRunRootServer->isChecked())
  GlobSet->NetModule->StartRootHttpServer(GlobSet->DefaultRootServerPort, GlobSet->ExternalJSROOT);  //does nothing if compilation flag is not set
}

void GlobalSettingsWindowClass::on_leJSROOT_editingFinished()
{
  GlobSet->ExternalJSROOT = ui->leJSROOT->text();
}
