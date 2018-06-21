//ANTS2 modules
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "geometrywindowclass.h"
#include "graphwindowclass.h"
#include "eventsdataclass.h"
#include "lrfwindow.h"
#include "reconstructionwindow.h"
#include "sensorlrfs.h"
#include "apmhub.h"
#include "materialinspectorwindow.h"
#include "outputwindow.h"
#include "guiutils.h"
#include "windownavigatorclass.h"
#include "exampleswindow.h"
#include "detectoraddonswindow.h"
#include "checkupwindowclass.h"
#include "CorrelationFilters.h"
#include "amaterialparticlecolection.h"
#include "detectorclass.h"
#include "simulationmanager.h"
#include "areconstructionmanager.h"
#include "particlesourcesclass.h"
#include "globalsettingsclass.h"
#include "globalsettingswindowclass.h"
#include "aconfiguration.h"
#include "amessage.h"
#include "alrfmoduleselector.h"
#include "gui/alrfwindow.h"
#include "anetworkmodule.h"
#include "awebsocketserverdialog.h"

#ifdef ANTS_FANN
#include "neuralnetworksmodule.h"
#include "neuralnetworkswindow.h"
#endif

#include <QVector>
#include <QScreen>
#include <QDebug>

//Root
#include "TApplication.h"
#include "TGaxis.h"

MainWindow::MainWindow(DetectorClass *Detector,
                       EventsDataClass *EventsDataHub,
                       TApplication *RootApp,
                       ASimulationManager *SimulationManager,
                       AReconstructionManager *ReconstructionManager,
                       ANetworkModule* Net,
                       TmpObjHubClass *TmpHub,
                       GlobalSettingsClass *GlobSet) :
    QMainWindow(), Detector(Detector), EventsDataHub(EventsDataHub), RootApp(RootApp),
    SimulationManager(SimulationManager), ReconstructionManager(ReconstructionManager),
    NetModule(Net), TmpHub(TmpHub), GlobSet(GlobSet),
    ui(new Ui::MainWindow)
{
    qDebug() << ">Main window constructor started";

    //aliases to use in GUI
    MpCollection = Detector->MpCollection;
    PMs = Detector->PMs;
    Config = Detector->Config;

    qDebug()<<">Creating user interface for the main window";
    ui->setupUi(this);
    this->setFixedSize(this->size());
    this->move(10,30); //default position    
    int minVer = ANTS2_MINOR;
    QString miv = QString::number(minVer);
    if (miv.length() == 1) miv = "0"+miv;
    int majVer = ANTS2_MAJOR;
    QString mav = QString::number(majVer);
    setWindowTitle("ANTS2_v"+mav+"."+miv);

    //adding Context menus
    ui->lwLoadedEventsFiles->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(ui->lwLoadedEventsFiles, SIGNAL(customContextMenuRequested(const QPoint&)),
        this, SLOT(LoadEventsListContextMenu(const QPoint&)));

    //setting up alias for particle sources container class - just for GUI
    ParticleSources = SimulationManager->ParticleSources;

    //interface windows
    qDebug()<<">Creating Examples Window";
    QWidget* w = new QWidget();
    ELwindow = new ExamplesWindow(w, this);
    ELwindow->move(100,100);    
    qDebug()<<">Creating Detector Add-ons window";  //created as child window, no delete on mainwin close!
    DAwindow = new DetectorAddOnsWindow(this, Detector);
    DAwindow->move(50,50);
    qDebug()<<">Creating Material Inspector Window";
    w = new QWidget();
    MIwindow = new MaterialInspectorWindow(w, this, Detector);
    MIwindow->move(50,50);
    qDebug()<<">Creating Output Window";
    w = new QWidget();
    Owindow = new OutputWindow(w, this, EventsDataHub);
    Owindow->move(600,580);
    qDebug()<<">Creating Reconstruction Window";
    w = new QWidget();
    Rwindow = new ReconstructionWindow(w, this, EventsDataHub);
    Rwindow->move(20,250);
    qDebug()<<">Creating LRF Window";
    w = new QWidget();
    lrfwindow = new LRFwindow(w, this, EventsDataHub);
    lrfwindow->move(25,25);
    qDebug()<<">Creating New LRF Window";
    w = new QWidget();
    newLrfWindow = new ALrfWindow(w, this, Detector->LRFs->getNewModule());    
#ifdef ANTS_FANN
    qDebug()<<">Creating NeuralNetworks Window";
    w = new QWidget();
    NNwindow = new NeuralNetworksWindow(w, this, EventsDataHub);
    NNwindow->move(25,25);
    QObject::connect(Rwindow,SIGNAL(cb3DreconstructionChanged(bool)),NNwindow,SLOT(onReconstruct3D(bool)));
    QObject::connect(Rwindow,SIGNAL(cbReconstructEnergyChanged(bool)),NNwindow,SLOT(onReconstructE(bool)));
#endif
    qDebug()<<">Creating check-up window"; //created as child window, no delete on mainwin close!
    CheckUpWindow = new CheckUpWindowClass(this, Detector);
    CheckUpWindow->move(50,50);
    qDebug()<<">Creating settings window";
    GlobSetWindow = new GlobalSettingsWindowClass(this);
    GlobSetWindow->move(50,50);
    qDebug()<<">Creating window navigator";
    w = new QWidget();
    WindowNavigator = new WindowNavigatorClass(w, this);
    WindowNavigator->move(700,50);
    qDebug()<<">Creating graph window";
    w = new QWidget();
    GraphWindow = new GraphWindowClass(w, this);
    GraphWindow->move(25,25);
    qDebug()<<">Creating geometry window";
    w = new QWidget();
    GeometryWindow = new GeometryWindowClass(w, this);
    GeometryWindow->move(25,25);
    qDebug()<<">Creating JavaScript window";
    createScriptWindow();
#ifdef __USE_ANTS_PYTHON__
    qDebug()<<">Creating Python script window";
    createPythonScriptWindow();
#endif
    ServerDialog = new AWebSocketServerDialog(this);
    qDebug()<<">All windows created";

    //root update cycle
    RootUpdateTimer = new QTimer(this);
    RootUpdateTimer->setInterval(100);
    QObject::connect(RootUpdateTimer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    RootUpdateTimer->start();
    qDebug()<<">Timer to refresh Root events started";

    // connect Config requests for Gui updates
    QObject::connect(Config, &AConfiguration::requestDetectorGuiUpdate, this, &MainWindow::onRequestDetectorGuiUpdate);
    QObject::connect(Config, SIGNAL(requestSimulationGuiUpdate()), this, SLOT(onRequestSimulationGuiUpdate()));
    QObject::connect(Config, SIGNAL(requestSelectFirstActiveParticleSource()), this, SLOT(selectFirstActiveParticleSource()));
    QObject::connect(Config, SIGNAL(requestReconstructionGuiUpdate()), Rwindow, SLOT(onRequestReconstructionGuiUpdate()));
    QObject::connect(Config, SIGNAL(requestLRFGuiUpdate()), lrfwindow, SLOT(onRequestGuiUpdate()));
    QObject::connect(Config, SIGNAL(NewConfigLoaded()), this, SLOT(onNewConfigLoaded()));
    QObject::connect(Config, &AConfiguration::requestGuiBusyStatusChange, WindowNavigator, &WindowNavigatorClass::ChangeGuiBusyStatus);

    QObject::connect(EventsDataHub, SIGNAL(loaded(int, int)), this, SLOT(updateLoaded(int, int)));
    QObject::connect(this, SIGNAL(RequestStopLoad()), EventsDataHub, SLOT(onRequestStopLoad()));
    QObject::connect(EventsDataHub, SIGNAL(requestClearKNNfilter()), ReconstructionManager, SLOT(onRequestClearKNNfilter()));
    QObject::connect(EventsDataHub, &EventsDataClass::cleared, this, &MainWindow::onRequestUpdateGuiForClearData);
    QObject::connect(EventsDataHub, SIGNAL(requestEventsGuiUpdate()), Rwindow, SLOT(onRequestEventsGuiUpdate())); 

    QObject::connect(ReconstructionManager, SIGNAL(ReconstructionFinished(bool, bool)), Rwindow, SLOT(onReconstructionFinished(bool, bool)));
    QObject::connect(ReconstructionManager, SIGNAL(RequestShowStatistics()), Rwindow, SLOT(ShowStatistics()));
    QObject::connect(ReconstructionManager, SIGNAL(UpdateReady(int,double)), Rwindow, SLOT(RefreshOnTimer(int,double)));

    QObject::connect(Rwindow, SIGNAL(StopRequested()), ReconstructionManager, SLOT(requestStop()));

    QObject::connect(Config, &AConfiguration::RequestClearParticleStack, this, &MainWindow::on_pbClearAllStack_clicked);

#ifdef ANTS_FANN
    QObject::connect(ReconstructionManager->ANNmodule,SIGNAL(status(QString)),NNwindow,SLOT(status(QString)));
    QObject::connect(NNwindow,SIGNAL(train_stop()),ReconstructionManager->ANNmodule,SLOT(train_stop()));
#endif

    //Busy status updates
    QObject::connect(WindowNavigator, SIGNAL(BusyStatusChanged(bool)), newLrfWindow, SLOT(onBusyStatusChanged(bool)));

    QObject::connect(SimulationManager->Runner, SIGNAL(updateReady(int, double)), this, SLOT(RefreshPhotSimOnTimer(int, double))); //Simulation interface refresh/update stuff
    QObject::connect(SimulationManager, SIGNAL(SimulationFinished()), this, SLOT(simulationFinished())); //Simulation finished

    DoNotUpdateGeometry = false; //control

    qDebug()<<">Loading default detector...";
    bool fLoadedDefaultDetector = MainWindow::startupDetector();
    qDebug()<<">Default detector configured";

    //Environment
    ui->fLoadProgress->setVisible(false);
    TGaxis::SetMaxDigits(3);  //Global setting for cern Root graphs!
     //Script window geometry
    ScriptWinX = this->x() + 100;
    ScriptWinY = this->y() + 100;
    ScriptWinW = 350;
    ScriptWinH = 380;

    //GUI updates
    qDebug() << ">Running GUI updates";
    //installing validators for edit boxes
      //double
    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
    foreach(QLineEdit *w, list) if (w->objectName().startsWith("led")) w->setValidator(dv);
      //int
    QIntValidator* iv  = new QIntValidator(this);
    iv->setBottom(0);
    foreach(QLineEdit *w, list) if (w->objectName().startsWith("lei")) w->setValidator(iv);
      //styles
    QString styleGrey = "QLabel { background-color: #E0E0E0; }";  //setting grey background color
    ui->laBackground->setStyleSheet(styleGrey);
    ui->laBackground_2->setStyleSheet(styleGrey);
    //QString styleWhite = "QLabel { background-color: #FbFbFb; }";  //setting grey background color
    //ui->laBackgroundPhotonSource->setStyleSheet(styleWhite);
    QString styleWhiteF = "QFrame { background-color: #FbFbFb; }";
    ui->fAdvOptionsSim->setStyleSheet(styleWhiteF);
      //fonts
    QFont ff = ui->tabwidMain->tabBar()->font();
    ff.setBold(true);
    ui->tabwidMain->tabBar()->setFont(ff);
      //pixmaps
    ui->labReloadRequired->setPixmap(Rwindow->RedIcon.pixmap(16,16));
    ui->labAdvancedOn->setPixmap(Rwindow->YellowIcon.pixmap(16,16));
    ui->labAdvancedOn->setVisible(false);
      //misc gui inits
    ui->swPMTvsSiPM->setCurrentIndex(ui->cobPMdeviceType->currentIndex());
    MainWindow::on_pbRefreshPMproperties_clicked(); //indication of PM properties
    ui->tabWidget->setCurrentIndex(0);
    MainWindow::on_pbElUpdateIndication_clicked();
    ui->twSourcePhotonsParticles->setCurrentIndex(0);
    MainWindow::PointSource_InitTabWidget();
    QList<QWidget*> invis;
    invis << ui->pbRefreshParticles << ui->pbOverride << ui->pbRefreshOverrides << ui->pbUpdatePreprocessingSettings
     << ui->pbRefreshStack << ui->pbShowPMsArrayRegularData << ui->pbRefreshPMArrayData << ui->pbUpdateElectronics
     << ui->pbRefreshPMproperties << ui->pbUpdatePMproperties << ui->pbRefreshMaterials << ui->pbStopLoad
     << ui->pbGunRefreshparticles << ui->pbIndPMshowInfo << ui->pbUpdateToFixedZ << ui->pbUpdateSimConfig
     << ui->pbUpdateToFullCustom << ui->pbElUpdateIndication << ui->pbUnlockGui
     << ui->pbInitializeScanFloodNoise << ui->pbUpdateScanFloodTabWidget << ui->fScanFloodTotProb
     << ui->fSecondaryScintLoadProfile << ui->pbUpdateSources << ui->pbUpdateSourcesIndication
     << ui->fLinkedParticle << ui->sbPMtype << ui->fUpperLowerArrays << ui->sbPMtypeForGroup
     << ui->pbRebuildDetector << ui->fReloadRequired << ui->pbYellow << ui->pbGDML << ui->fGunMultipleEvents;
    for (int i=0;i<invis.length();i++) invis[i]->setVisible(false);
    ui->frLimitNodesTo->setVisible( ui->twSingleScan->currentIndex()!=0 );
    QList<QWidget*> disables;
    disables << ui->fGunParticle << ui->fFixedDir << ui->fFixedWorldSize;
    for (int i=0;i<disables.length();i++) disables[i]->setEnabled(false);
    ui->fWaveOptions->setEnabled(ui->cbWaveResolved->isChecked());
    ui->fWaveTests->setEnabled(ui->cbWaveResolved->isChecked());
    ui->fTime->setEnabled(ui->cbTimeResolved->isChecked());
    ui->fPointSource_Wave->setEnabled(ui->cbWaveResolved->isChecked());
    ui->fPointSource_Time->setEnabled(ui->cbTimeResolved->isChecked());
    ui->swPointSourceWaveTime->setCurrentIndex(ui->cobDirectlyOrFromMaterial->currentIndex());
    ui->fAngular->setEnabled(ui->cbAngularSensitive->isChecked());    
    ui->fScanSecond->setEnabled(ui->cbSecondAxis->isChecked());
    ui->fScanThird->setEnabled(ui->cbThirdAxis->isChecked());
    ui->fPreprocessing->setEnabled(ui->cbPMsignalPreProcessing->isChecked());    
    qDebug() << ">GUI initialized";

    //change font size for all windows
    if (this->font().pointSize() != GlobSet->FontSize) setFontSizeAllWindows(GlobSet->FontSize);
    qDebug() << ">Font size adjusted";

    //menu properties
    QString mss = ui->menuFile->styleSheet();
    mss += "; QMenu::tooltip {wakeDelay: 1;}";
    ui->menuFile->setStyleSheet(mss);
    ui->menuFile->setToolTipsVisible(true);
    ui->menuFile->setToolTipDuration(1000);

    bool fShowGeom;
    if (GlobSet->SaveLoadWindows)
    {
      MainWindow::on_actionLoad_positions_and_status_of_all_windows_triggered();
      fShowGeom = GeometryWindow->isVisible();
    }
    else
    {
        WindowNavigator->show();
        fShowGeom = true;
        AssureWidgetIsWithingVisibleArea(this);
    }
    ui->actionSave_Load_windows_status_on_Exit_Init->setChecked(GlobSet->SaveLoadWindows);

    qDebug() << ">Init for Output window";
    Owindow->InitWindow();
    Owindow->resize(Owindow->width()+1, Owindow->height());
    Owindow->resize(Owindow->width()-1, Owindow->height());

    qDebug() << ">Init for Reconstruction window...";
    Rwindow->InitWindow();

    qDebug()<<">Showing geometry";
    GeometryWindow->show();
    GeometryWindow->resize(GeometryWindow->width()+1, GeometryWindow->height());
    GeometryWindow->resize(GeometryWindow->width()-1, GeometryWindow->height());
    QThread::msleep(50);
    GeometryWindow->ShowGeometry(false);
    if (!fShowGeom) GeometryWindow->hide();

    ui->cobScatteringModel->setCurrentIndex(1); //default to Lambertian back
    ui->cobSSO_ScatterModel->setCurrentIndex(1); //default to Lambertian back
    //MainWindow::on_pbRefreshOverrides_clicked();  //already in load detector
    MainWindow::updateCOBsWithPMtypeNames();

    if (!fLoadedDefaultDetector)
      message("Startup detector NOT found, dummy default detector is loaded", this);
    qDebug()<<">Main window initialization complete";
}
