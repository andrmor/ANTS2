#include "ajavascriptmanager.h"
#include "ui_mainwindow.h"
#include "aglobalsettings.h"
#include "detectorclass.h"
#include "eventsdataclass.h"
#include "interfacetoglobscript.h"
#include "scriptminimizer.h"
#include "histgraphinterfaces.h"
#include "ageo_si.h"
#include "adepo_si.h"
#include "aconfiguration.h"
#include "areconstructionmanager.h"
#include "simulationmanager.h"
#include "aweb_si.h"
#include "aserver_si.h"
#include "anetworkmodule.h"
#include "aphoton_si.h"
#include "athreads_si.h"
#include "agui_si.h"
#include "atree_si.h"
#include "atracklog_si.h"
#include "asim_si.h"
#include "aconfig_si.h"

#include "mainwindow.h"
#include "graphwindowclass.h"
#include "geometrywindowclass.h"
#include "amsg_si.h"
#include "reconstructionwindow.h"
#include "windownavigatorclass.h"
#include "lrfwindow.h"
#include "ascriptwindow.h"
#include "checkupwindowclass.h"

#ifdef ANTS_FLANN
  #include "aknn_si.h"
#endif

#ifdef ANTS_FANN
  #include "AAnn_SI.h"
#endif

#include <QDebug>

void MainWindow::createScriptWindow()
{
    QWidget* w = new QWidget();
    AJavaScriptManager* SM = new AJavaScriptManager(Detector->RandGen);
    ScriptWindow = new AScriptWindow(SM, false, w); //transfer ownership of SM
    ScriptWindow->move(25,25);
    connect(ScriptWindow, &AScriptWindow::WindowShown, WindowNavigator, &WindowNavigatorClass::ShowWindowTriggered);
    connect(ScriptWindow, &AScriptWindow::WindowHidden, WindowNavigator, &WindowNavigatorClass::HideWindowTriggered);
    //connect(SM, &AScriptManager::reportProgress, WindowNavigator, &WindowNavigatorClass::setProgress);
    connect(SM, &AScriptManager::reportProgress, ScriptWindow, &AScriptWindow::onProgressChanged);
    NetModule->SetScriptManager(SM);

    // interface objects are owned after this by the ScriptManager!

    ScriptWindow->RegisterCoreInterfaces();

    AThreads_SI* threads = new AThreads_SI(SM);
    ScriptWindow->RegisterInterface(threads, "threads");

    AConfig_SI* conf = new AConfig_SI(Config);
    QObject::connect(conf, SIGNAL(requestReadRasterGeometry()), GeometryWindow, SLOT(readRasterWindowProperties()));
    ScriptWindow->RegisterInterface(conf, "config");

    AGeo_SI* geo = new AGeo_SI(Detector);
    connect(geo, SIGNAL(requestShowCheckUpWindow()), CheckUpWindow, SLOT(showNormal()));
    ScriptWindow->RegisterInterface(geo, "geo");

    AMini_JavaScript_SI* mini = new AMini_JavaScript_SI(SM);
    ScriptWindow->RegisterInterface(mini, "mini");  //mini should be before sim to handle abort correctly

    AInterfaceToData* dat = new AInterfaceToData(Config, EventsDataHub);
    QObject::connect(dat, SIGNAL(RequestEventsGuiUpdate()), Rwindow, SLOT(onRequestEventsGuiUpdate()));
    ScriptWindow->RegisterInterface(dat, "events");

    ASim_SI* sim = new ASim_SI(SimulationManager, EventsDataHub, Config, GlobSet.RecNumTreads);
    QObject::connect(sim, SIGNAL(requestStopSimulation()), SimulationManager, SLOT(StopSimulation()));
    ScriptWindow->RegisterInterface(sim, "sim");

    InterfaceToReconstructor* rec = new InterfaceToReconstructor(ReconstructionManager, Config, EventsDataHub, TmpHub, GlobSet.RecNumTreads);
    QObject::connect(rec, SIGNAL(RequestStopReconstruction()), ReconstructionManager, SLOT(requestStop()));
    QObject::connect(rec, SIGNAL(RequestUpdateGuiForManifest()), Rwindow, SLOT(onManifestItemsGuiUpdate()));
    ScriptWindow->RegisterInterface(rec, "rec");

    AInterfaceToLRF* lrf = new AInterfaceToLRF(Config, EventsDataHub);
    ScriptWindow->RegisterInterface(lrf, "lrf");
    ALrfScriptInterface* newLrf = new ALrfScriptInterface(Detector, EventsDataHub);
    ScriptWindow->RegisterInterface(newLrf, "newLrf");

    AInterfaceToPMs* pmS = new AInterfaceToPMs(Config);
    ScriptWindow->RegisterInterface(pmS, "pms");

    AInterfaceToGraph* graph = new AInterfaceToGraph(TmpHub);
    ScriptWindow->RegisterInterface(graph, "graph");

    AInterfaceToHist* hist = new AInterfaceToHist(TmpHub);
    ScriptWindow->RegisterInterface(hist, "hist");

    ATree_SI* tree = new ATree_SI(TmpHub);
    ScriptWindow->RegisterInterface(tree, "tree");
    connect(tree, &ATree_SI::RequestTreeDraw, GraphWindow, &GraphWindowClass::DrawTree);

    AMsg_SI* txt = new AMsg_SI(SM, ScriptWindow);
    ScriptWindow->RegisterInterface(txt, "msg");

    AWeb_SI* web = new AWeb_SI(EventsDataHub);
    QObject::connect(web, &AWeb_SI::showTextOnMessageWindow, txt, &AMsg_SI::Append); // make sure this line is after AInterfaceToMessageWindow init
    QObject::connect(web, &AWeb_SI::clearTextOnMessageWindow, txt, &AMsg_SI::Clear); // make sure this line is after AInterfaceToMessageWindow init
    ScriptWindow->RegisterInterface(web, "web");

    AServer_SI* server = new AServer_SI(*NetModule->WebSocketServer, EventsDataHub);
    ScriptWindow->RegisterInterface(server, "server");

    APhoton_SI* photon = new APhoton_SI(Config, EventsDataHub);
    ScriptWindow->RegisterInterface(photon, "photon");

    ADepo_SI* depo = new ADepo_SI(Detector, EventsDataHub);
    ScriptWindow->RegisterInterface(depo, "depo");

    ATrackLog_SI* pth = new ATrackLog_SI(*EventsDataHub);
    ScriptWindow->RegisterInterface(pth, "tracklog");

#ifdef ANTS_FLANN
    AKnn_SI* knn = new AKnn_SI(ReconstructionManager->KNNmodule);
    ScriptWindow->RegisterInterface(knn, "knn");
#endif

#ifdef ANTS_FANN
    //AAnn_SI* ann = new AAnn_SI();
    //ScriptWindow->RegisterInterface(ann, "ann");
#endif

    // Interfaces which rely on MainWindow

    AGeoWin_SI* geowin = new AGeoWin_SI(this, SimulationManager);
    ScriptWindow->RegisterInterface(geowin, "geowin");

    InterfaceToGraphWin* grwin = new InterfaceToGraphWin(this);
    ScriptWindow->RegisterInterface(grwin, "grwin");

    AGui_SI* gui = new AGui_SI(SM);
    ScriptWindow->RegisterInterface(gui, "gui");

    AInterfaceToOutputWin* out = new AInterfaceToOutputWin(this);
    ScriptWindow->RegisterInterface(out, "outwin");

    // window inits
    ScriptWindow->SetShowEvaluationResult(true);

    QObject::connect(ScriptWindow, SIGNAL(onStart()), this, SLOT(onGlobalScriptStarted()));
    QObject::connect(ScriptWindow, SIGNAL(success(QString)), this, SLOT(onGlobalScriptFinished()));
    QObject::connect(ScriptWindow, SIGNAL(RequestDraw(TObject*,QString,bool)), GraphWindow, SLOT(DrawStrOpt(TObject*,QString,bool)));

    ScriptWindow->UpdateGui();
}

void MainWindow::onGlobalScriptStarted()
{
  WindowNavigator->BusyOn();
  qApp->processEvents();
}

void MainWindow::onGlobalScriptFinished()
{
  Config->AskForAllGuiUpdate();  
  //if (GenScriptWindow) GenScriptWindow->updateJsonTree();
  if (ScriptWindow) ScriptWindow->updateJsonTree();
  WindowNavigator->BusyOff();
}
