#include "ajavascriptmanager.h"
#include "ui_mainwindow.h"
#include "aglobalsettings.h"
#include "detectorclass.h"
#include "eventsdataclass.h"
#include "interfacetoglobscript.h"
#include "scriptminimizer.h"
#include "histgraphinterfaces.h"
#include "ainterfacetoaddobjscript.h"
#include "ainterfacetodeposcript.h"
#include "aconfiguration.h"
#include "areconstructionmanager.h"
#include "simulationmanager.h"
#include "ainterfacetowebsocket.h"
#include "awebserverinterface.h"
#include "anetworkmodule.h"
#include "ainterfacetophotonscript.h"
#include "ainterfacetomultithread.h"
#include "ainterfacetoguiscript.h"
#include "ainterfacetottree.h"
#include "aparticletrackinghistoryinterface.h"

#include "mainwindow.h"
#include "graphwindowclass.h"
#include "geometrywindowclass.h"
#include "ainterfacetomessagewindow.h"
#include "reconstructionwindow.h"
#include "windownavigatorclass.h"
#include "lrfwindow.h"
#include "ascriptwindow.h"
#include "checkupwindowclass.h"

#ifdef ANTS_FLANN
  #include "ainterfacetoknnscript.h"
#endif

#ifdef ANTS_FANN
  #include "ainterfacetoannscript.h"
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

    AInterfaceToMultiThread* threads = new AInterfaceToMultiThread(SM);
    ScriptWindow->RegisterInterface(threads, "threads");

    AInterfaceToConfig* conf = new AInterfaceToConfig(Config);
    QObject::connect(conf, SIGNAL(requestReadRasterGeometry()), GeometryWindow, SLOT(readRasterWindowProperties()));
    ScriptWindow->RegisterInterface(conf, "config");

    AInterfaceToAddObjScript* geo = new AInterfaceToAddObjScript(Detector);
    connect(geo, SIGNAL(requestShowCheckUpWindow()), CheckUpWindow, SLOT(showNormal()));
    ScriptWindow->RegisterInterface(geo, "geo");

    AInterfaceToMinimizerJavaScript* mini = new AInterfaceToMinimizerJavaScript(SM);
    ScriptWindow->RegisterInterface(mini, "mini");  //mini should be before sim to handle abort correctly

    AInterfaceToData* dat = new AInterfaceToData(Config, EventsDataHub);
    QObject::connect(dat, SIGNAL(RequestEventsGuiUpdate()), Rwindow, SLOT(onRequestEventsGuiUpdate()));
    ScriptWindow->RegisterInterface(dat, "events");

    InterfaceToSim* sim = new InterfaceToSim(SimulationManager, EventsDataHub, Config, GlobSet.RecNumTreads);
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

    AInterfaceToTTree* tree = new AInterfaceToTTree(TmpHub);
    ScriptWindow->RegisterInterface(tree, "tree");
    connect(tree, &AInterfaceToTTree::RequestTreeDraw, GraphWindow, &GraphWindowClass::DrawTree);

    AInterfaceToMessageWindow* txt = new AInterfaceToMessageWindow(SM, ScriptWindow);
    ScriptWindow->RegisterInterface(txt, "msg");

    AInterfaceToWebSocket* web = new AInterfaceToWebSocket(EventsDataHub);
    QObject::connect(web, &AInterfaceToWebSocket::showTextOnMessageWindow, txt, &AInterfaceToMessageWindow::Append); // make sure this line is after AInterfaceToMessageWindow init
    QObject::connect(web, &AInterfaceToWebSocket::clearTextOnMessageWindow, txt, &AInterfaceToMessageWindow::Clear); // make sure this line is after AInterfaceToMessageWindow init
    ScriptWindow->RegisterInterface(web, "web");

    AWebServerInterface* server = new AWebServerInterface(*NetModule->WebSocketServer, EventsDataHub);
    ScriptWindow->RegisterInterface(server, "server");

    AInterfaceToPhotonScript* photon = new AInterfaceToPhotonScript(Config, EventsDataHub);
    ScriptWindow->RegisterInterface(photon, "photon");

    AInterfaceToDepoScript* depo = new AInterfaceToDepoScript(Detector, EventsDataHub);
    ScriptWindow->RegisterInterface(depo, "depo");

    AParticleTrackingHistoryInterface* pth = new AParticleTrackingHistoryInterface(*EventsDataHub);
    ScriptWindow->RegisterInterface(pth, "tracklog");

#ifdef ANTS_FLANN
    AInterfaceToKnnScript* knn = new AInterfaceToKnnScript(ReconstructionManager->KNNmodule);
    ScriptWindow->RegisterInterface(knn, "knn");
#endif

#ifdef ANTS_FANN
    //AInterfaceToANNScript* ann = new AInterfaceToANNScript();
    //ScriptWindow->RegisterInterface(ann, "ann");
#endif

    // Interfaces which rely on MainWindow

    InterfaceToGeoWin* geowin = new InterfaceToGeoWin(this, TmpHub);
    ScriptWindow->RegisterInterface(geowin, "geowin");

    InterfaceToGraphWin* grwin = new InterfaceToGraphWin(this);
    ScriptWindow->RegisterInterface(grwin, "grwin");

    AInterfaceToGuiScript* gui = new AInterfaceToGuiScript(SM);
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
