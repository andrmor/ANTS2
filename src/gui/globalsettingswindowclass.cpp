#include "globalsettingswindowclass.h"
#include "ui_globalsettingswindowclass.h"
#include "mainwindow.h"
#include "ajavascriptmanager.h"
#include "ascriptwindow.h"
#include "aglobalsettings.h"
#include "detectorclass.h"
#include "amessage.h"
#include "anetworkmodule.h"
#include "geometrywindowclass.h"
#include "ainterfacetogstylescript.h"

//Qt
#include <QFileDialog>
#include <QDesktopServices>
#include <QDebug>
#include <QHostAddress>

#include "TGeoManager.h"

GlobalSettingsWindowClass::GlobalSettingsWindowClass(MainWindow *parent) :
  QMainWindow(parent),
  ui(new Ui::GlobalSettingsWindowClass)
{
  MW = parent;  
  ui->setupUi(this);

  Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
  windowFlags |= Qt::WindowCloseButtonHint;
  windowFlags |= Qt::WindowStaysOnTopHint;
  this->setWindowFlags( windowFlags );

  updateGUI();
  QObject::connect(MW->GlobSet.getNetworkModule(), &ANetworkModule::StatusChanged, this, &GlobalSettingsWindowClass::updateNetGui);

  if (MW->GlobSet.fRunRootServerOnStart)
    MW->GlobSet.getNetworkModule()->StartRootHttpServer(MW->GlobSet.DefaultRootServerPort, MW->GlobSet.ExternalJSROOT);  //does nothing if compilation flag is not set
}

GlobalSettingsWindowClass::~GlobalSettingsWindowClass()
{
   delete ui; ui = 0;
}

void GlobalSettingsWindowClass::updateGUI()
{
  ui->leLibPMtypes->setText(MW->GlobSet.LibPMtypes);
  ui->leLibMaterials->setText(MW->GlobSet.LibMaterials);
  ui->leLibParticleSources->setText(MW->GlobSet.LibParticleSources);
  ui->leLibScripts->setText(MW->GlobSet.LibScripts);

  ui->leWorkingDir->setText(MW->GlobSet.TmpDir);

  ui->sbFontSize->setValue(MW->GlobSet.FontSize);

  //ui->rbNever->setChecked(MW->GlobSet.NeverSaveOnExit);
  //ui->rbAlways->setChecked(MW->GlobSet.AlwaysSaveOnExit);
  //ui->rbAskMe->setChecked(!MW->GlobSet.NeverSaveOnExit && !MW->GlobSet.AlwaysSaveOnExit);

  ui->cbOpenImageExternalEditor->setChecked(MW->GlobSet.fOpenImageExternalEditor);

  ui->sbLogPrecision->setValue(MW->GlobSet.TextLogPrecision);

  ui->sbNumBinsHistogramsX->setValue(MW->GlobSet.BinsX);
  ui->sbNumBinsHistogramsY->setValue(MW->GlobSet.BinsY);
  ui->sbNumBinsHistogramsZ->setValue(MW->GlobSet.BinsZ);

  ui->sbNumPointsFunctionX->setValue(MW->GlobSet.FunctionPointsX);
  ui->sbNumPointsFunctionY->setValue(MW->GlobSet.FunctionPointsY);

  ui->sbNumSegments->setValue(MW->GlobSet.NumSegments);

  ui->sbNumTreads->setValue(MW->GlobSet.NumThreads);
  ui->sbRecNumTreads->setValue(MW->GlobSet.RecNumTreads);

  ui->cbSaveRecAsTree_IncludePMsignals->setChecked(MW->GlobSet.RecTreeSave_IncludePMsignals);
  ui->cbSaveRecAsTree_IncludeRho->setChecked(MW->GlobSet.RecTreeSave_IncludeRho);
  ui->cbSaveRecAsTree_IncludeTrue->setChecked(MW->GlobSet.RecTreeSave_IncludeTrue);

  ui->cbSaveSimAsText_IncludeNumPhotons->setChecked(MW->GlobSet.SimTextSave_IncludeNumPhotons);
  ui->cbSaveSimAsText_IncludePositions->setChecked(MW->GlobSet.SimTextSave_IncludePositions);

  ui->leGeant4exec->setText(MW->GlobSet.G4antsExec);
  ui->leGeant4ExchangeFolder->setText(MW->GlobSet.G4ExchangeFolder);

  updateNetGui();
}

void GlobalSettingsWindowClass::updateNetGui()
{
  ui->leWebSocketIP->setText(MW->GlobSet.DefaultWebSocketIP);
  ui->leWebSocketPort->setText(QString::number(MW->GlobSet.DefaultWebSocketPort));

  ANetworkModule* Net = MW->GlobSet.getNetworkModule();

  bool fWebSocketRunning = Net->isWebSocketServerRunning();
  ui->cbRunWebSocketServer->setChecked( fWebSocketRunning );
  if (fWebSocketRunning)
    {
      int port = Net->getWebSocketPort();
      ui->leWebSocketPort->setText(QString::number(port));
      ui->leWebSocketURL->setText(Net->getWebSocketServerURL());
    }

  bool fRootServerRunning = Net->isRootServerRunning();
  ui->cbRunRootServer->setChecked( fRootServerRunning );
  ui->cbAutoRunRootServer->setChecked( MW->GlobSet.fRunRootServerOnStart );

#ifdef USE_ROOT_HTML
  if (fRootServerRunning)
    {
      ui->leJSROOT->setText( Net->getJSROOTstring());
      int port = Net->getRootServerPort();
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

bool GlobalSettingsWindowClass::event(QEvent *event)
{
    if (event->type() == QEvent::WindowActivate) updateGUI();

    return QMainWindow::event(event);
}

void GlobalSettingsWindowClass::on_pbgStyleScript_clicked()
{
    MW->extractGeometryOfLocalScriptWindow();
    delete MW->GenScriptWindow; MW->GenScriptWindow = 0;

    AJavaScriptManager* jsm = new AJavaScriptManager(MW->Detector->RandGen);
    MW->GenScriptWindow = new AScriptWindow(jsm, true, this);

    QString example = QString("//see https://root.cern.ch/doc/master/classTStyle.html\n"
                              "\n"
                              "//try, e.g.:\n"
                              "//SetOptStat(\"ei\") //\"nemr\" is redault");
    MW->GenScriptWindow->ConfigureForLightMode(&MW->GlobSet.RootStyleScript,
                              "Script to set ROOT's gStyle",
                              example);

    GStyleInterface = new AInterfaceToGStyleScript();
    MW->GenScriptWindow->RegisterInterfaceAsGlobal(GStyleInterface);
    MW->GenScriptWindow->UpdateGui();

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
  MW->GenScriptWindow->SetScript(&MW->GlobSet.RootStyleScript);
  MW->GenScriptWindow->SetStarterDir(MW->MW->GlobSet.LibScripts);

  //define what to do on evaluation success
  //connect(MW->GenScriptWindow, SIGNAL(success(QString)), this, SLOT(HolesScriptSuccess()));
  //if needed. connect signals of the interface object with the required slots of any ANTS2 objects
  MW->GenScriptWindow->show();
  */
}

void GlobalSettingsWindowClass::on_pbChoosePMtypeLib_clicked()
{
  QString starter = (MW->GlobSet.LibPMtypes.isEmpty()) ? MW->GlobSet.LastOpenDir : MW->GlobSet.LibPMtypes;
  QString dir = QFileDialog::getExistingDirectory(this, "Select directory for PM types",starter,QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (dir.isEmpty()) return;
  MW->GlobSet.LibPMtypes = dir;
  ui->leLibPMtypes->setText(dir);
}
void GlobalSettingsWindowClass::on_leLibPMtypes_editingFinished()
{
  MW->GlobSet.LibPMtypes = ui->leLibPMtypes->text();
}

void GlobalSettingsWindowClass::on_pbChooseMaterialLib_clicked()
{
  QString starter = (MW->GlobSet.LibMaterials.isEmpty()) ? MW->GlobSet.LastOpenDir : MW->GlobSet.LibMaterials;
  QString dir = QFileDialog::getExistingDirectory(this, "Select directory for materials",starter,QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (dir.isEmpty()) return;
  MW->GlobSet.LibMaterials = dir;
  ui->leLibMaterials->setText(dir);
}
void GlobalSettingsWindowClass::on_leLibMaterials_editingFinished()
{
  MW->GlobSet.LibMaterials = ui->leLibMaterials->text();
}

void GlobalSettingsWindowClass::on_pbChooseParticleSourcesLib_clicked()
{
  QString starter = (MW->GlobSet.LibParticleSources.isEmpty()) ? MW->GlobSet.LastOpenDir : MW->GlobSet.LibParticleSources;
  QString dir = QFileDialog::getExistingDirectory(this, "Select directory for particle sources",starter,QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (dir.isEmpty()) return;
  MW->GlobSet.LibParticleSources = dir;
  ui->leLibParticleSources->setText(dir);
}
void GlobalSettingsWindowClass::on_leLibParticleSources_editingFinished()
{
  MW->GlobSet.LibParticleSources = ui->leLibParticleSources->text();
}

void GlobalSettingsWindowClass::on_pbChooseScriptsLib_clicked()
{
  QString starter = (MW->GlobSet.LibScripts.isEmpty()) ? MW->GlobSet.LastOpenDir : MW->GlobSet.LibScripts;
  QString dir = QFileDialog::getExistingDirectory(this, "Select directory for scripts",starter,QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (dir.isEmpty()) return;
  MW->GlobSet.LibScripts = dir;
  ui->leLibScripts->setText(dir);
}
void GlobalSettingsWindowClass::on_leLibScripts_editingFinished()
{
  MW->GlobSet.LibScripts = ui->leLibScripts->text();
}

void GlobalSettingsWindowClass::on_pbChangeWorkingDir_clicked()
{
  QString starter = MW->GlobSet.TmpDir;
  QString dir = QFileDialog::getExistingDirectory(this, "Select working directory (temporary files are saved here)",starter,QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (dir.isEmpty()) return;
  MW->GlobSet.TmpDir = dir;
  QDir::setCurrent(MW->GlobSet.TmpDir);
  ui->leWorkingDir->setText(dir);
}
void GlobalSettingsWindowClass::on_leWorkingDir_editingFinished()
{
   QString dir = ui->leWorkingDir->text();
   if ( QDir(dir).exists() )
     {
       MW->GlobSet.TmpDir = dir;
       QDir::setCurrent(MW->GlobSet.TmpDir);
       ui->leWorkingDir->setText(dir);
     }
   else
     {
       ui->leWorkingDir->setText(MW->GlobSet.TmpDir);
       message("Dir does not exists!", this);
     }
}

void GlobalSettingsWindowClass::on_sbFontSize_editingFinished()
{
    MW->GlobSet.FontSize = ui->sbFontSize->value();
    MW->setFontSizeAllWindows(MW->GlobSet.FontSize);
}

void GlobalSettingsWindowClass::on_rbAlways_toggled(bool /*checked*/)
{
  //MW->GlobSet.AlwaysSaveOnExit = checked;
}

void GlobalSettingsWindowClass::on_rbNever_toggled(bool /*checked*/)
{
  //MW->GlobSet.NeverSaveOnExit = checked;
}

void GlobalSettingsWindowClass::on_cbOpenImageExternalEditor_clicked(bool checked)
{
  MW->GlobSet.fOpenImageExternalEditor = checked;
}

void GlobalSettingsWindowClass::on_sbLogPrecision_editingFinished()
{
  MW->GlobSet.TextLogPrecision = ui->sbLogPrecision->value();
}

void GlobalSettingsWindowClass::on_sbNumBinsHistogramsX_editingFinished()
{
  MW->GlobSet.BinsX = ui->sbNumBinsHistogramsX->value();
}

void GlobalSettingsWindowClass::on_sbNumBinsHistogramsY_editingFinished()
{
  MW->GlobSet.BinsY = ui->sbNumBinsHistogramsY->value();
}

void GlobalSettingsWindowClass::on_sbNumBinsHistogramsZ_editingFinished()
{
  MW->GlobSet.BinsZ = ui->sbNumBinsHistogramsZ->value();
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
      what = MW->GlobSet.AntsBaseDir;
      break;
    case 1:
      what = MW->GlobSet.ConfigDir;
      break;
    case 2:
      what = MW->GlobSet.LibScripts;
      break;
    case 3:
      what = MW->GlobSet.LastOpenDir;
      break;
    case 4:
      what = MW->GlobSet.ExamplesDir;
      break;
    }
  QDesktopServices::openUrl(QUrl("file:///"+what, QUrl::TolerantMode));
}

void GlobalSettingsWindowClass::on_sbNumSegments_editingFinished()
{
    MW->GlobSet.NumSegments = ui->sbNumSegments->value();
    MW->Detector->GeoManager->SetNsegments(MW->GlobSet.NumSegments);
    MW->GeometryWindow->ShowGeometry(false);
}

void GlobalSettingsWindowClass::on_sbNumPointsFunctionX_editingFinished()
{
   MW->GlobSet.FunctionPointsX = ui->sbNumPointsFunctionX->value();
}

void GlobalSettingsWindowClass::on_sbNumPointsFunctionY_editingFinished()
{
   MW->GlobSet.FunctionPointsY = ui->sbNumPointsFunctionY->value();
}

void GlobalSettingsWindowClass::on_sbNumTreads_valueChanged(int arg1)
{
    ui->fMultiDisabled->setVisible(arg1 == 0);
}

void GlobalSettingsWindowClass::on_sbNumTreads_editingFinished()
{
    MW->GlobSet.NumThreads = ui->sbNumTreads->value();
}

void GlobalSettingsWindowClass::on_cbSaveRecAsTree_IncludePMsignals_clicked(bool checked)
{
    MW->GlobSet.RecTreeSave_IncludePMsignals = checked;
}

void GlobalSettingsWindowClass::on_cbSaveRecAsTree_IncludeRho_clicked(bool checked)
{
    MW->GlobSet.RecTreeSave_IncludeRho = checked;
}

void GlobalSettingsWindowClass::on_cbSaveRecAsTree_IncludeTrue_clicked(bool checked)
{
    MW->GlobSet.RecTreeSave_IncludeTrue = checked;
}

void GlobalSettingsWindowClass::on_cbRunWebSocketServer_clicked(bool checked)
{
  ANetworkModule* Net = MW->GlobSet.getNetworkModule();
  Net->StopWebSocketServer();

  if (checked)
      Net->StartWebSocketServer(QHostAddress(MW->GlobSet.DefaultWebSocketIP), MW->GlobSet.DefaultWebSocketPort);
}

void GlobalSettingsWindowClass::on_leWebSocketPort_editingFinished()
{
  int oldp = MW->GlobSet.DefaultWebSocketPort;
  int newp = ui->leWebSocketPort->text().toInt();

  if (oldp != newp)
  {
      MW->GlobSet.DefaultWebSocketPort = newp;
      ui->cbRunWebSocketServer->setChecked(false);
      MW->GlobSet.DefaultWebSocketPort = newp;
      ui->leWebSocketPort->setText(QString::number(newp));
  }
}

void GlobalSettingsWindowClass::on_leWebSocketIP_editingFinished()
{
    QString newIP = ui->leWebSocketIP->text();
    if (newIP == MW->GlobSet.DefaultWebSocketIP) return;

    QHostAddress ip = QHostAddress(newIP);
    if (ip.isNull())
    {
        ui->leWebSocketIP->setText(MW->GlobSet.DefaultWebSocketIP);
        message("Bad format of IP: use, e.g., 127.0.0.1", this);
    }
    else
    {
        MW->GlobSet.DefaultWebSocketIP = newIP;
        ui->cbRunWebSocketServer->setChecked(false);
    }
}

void GlobalSettingsWindowClass::on_cbRunRootServer_clicked(bool checked)
{
    ANetworkModule* Net = MW->GlobSet.getNetworkModule();

    if (checked)
      Net->StartRootHttpServer(MW->GlobSet.DefaultRootServerPort, MW->GlobSet.ExternalJSROOT);  //does nothing if compilation flag is not set
    else
      Net->StopRootHttpServer();
}

void GlobalSettingsWindowClass::on_cbAutoRunRootServer_clicked()
{
  MW->GlobSet.fRunRootServerOnStart = ui->cbAutoRunRootServer->isChecked();
}

void GlobalSettingsWindowClass::on_leRootServerPort_editingFinished()
{
  int oldp = MW->GlobSet.DefaultRootServerPort;
  int newp = ui->leRootServerPort->text().toInt();
  if (oldp == newp) return;
  MW->GlobSet.DefaultRootServerPort = newp;

  ui->cbRunRootServer->setChecked(false);
}

void GlobalSettingsWindowClass::on_leJSROOT_editingFinished()
{
  MW->GlobSet.ExternalJSROOT = ui->leJSROOT->text();
}

void GlobalSettingsWindowClass::on_cbRunWebSocketServer_toggled(bool checked)
{
    if (!checked) ui->leWebSocketURL->clear();
}

void GlobalSettingsWindowClass::on_cbSaveSimAsText_IncludeNumPhotons_clicked(bool checked)
{
    MW->GlobSet.SimTextSave_IncludeNumPhotons = checked;
}

void GlobalSettingsWindowClass::on_cbSaveSimAsText_IncludePositions_clicked(bool checked)
{
    MW->GlobSet.SimTextSave_IncludePositions = checked;
}

void GlobalSettingsWindowClass::on_pbGeant4exec_clicked()
{
    QString fn = QFileDialog::getOpenFileName(this, "G4ants executable", MW->GlobSet.G4antsExec);
    if (fn.isEmpty()) return;
    ui->leGeant4exec->setText(fn);
    MW->GlobSet.G4antsExec = fn;
}

void GlobalSettingsWindowClass::on_leGeant4exec_editingFinished()
{
    MW->GlobSet.G4antsExec = ui->leGeant4exec->text();
}

void GlobalSettingsWindowClass::on_pbGeant4ExchangeFolder_clicked()
{
    QString starter = MW->GlobSet.G4ExchangeFolder;
    QString dir = QFileDialog::getExistingDirectory(this, "Select folder for Ants2<->Geant4 file exchange",starter,QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) return;
    ui->leGeant4ExchangeFolder->setText(dir);
    MW->GlobSet.G4ExchangeFolder = ui->leGeant4ExchangeFolder->text();
}

void GlobalSettingsWindowClass::on_leGeant4ExchangeFolder_editingFinished()
{
    MW->GlobSet.G4ExchangeFolder = ui->leGeant4ExchangeFolder->text();
}
