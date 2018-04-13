#include "mainwindow.h"
#include "ajavascriptmanager.h"
#include "ui_mainwindow.h"
#include "genericscriptwindowclass.h"
#include "detectorclass.h"
#include "eventsdataclass.h"
#include "globalsettingsclass.h"
#include "interfacetoglobscript.h"
#include "ainterfacetomessagewindow.h"
#include "scriptminimizer.h"
#include "histgraphinterfaces.h"
#include "localscriptinterfaces.h"
#include "ainterfacetodeposcript.h"
#include "graphwindowclass.h"
#include "geometrywindowclass.h"
#include "aconfiguration.h"
#include "reconstructionwindow.h"
#include "areconstructionmanager.h"
#include "windownavigatorclass.h"
#include "simulationmanager.h"
#include "lrfwindow.h"
#include "ascriptwindow.h"
#include "checkupwindowclass.h"
#include "ainterfacetowebsocket.h"
#include "anetworkmodule.h"
#include "ainterfacetophotonscript.h"
#include "ainterfacetomultithread.h"
#include "ainterfacetoguiscript.h"
#include "ainterfacetottree.h"

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
    ScriptWindow = new AScriptWindow(SM, GlobSet, w); //transfer ownership of SM
    ScriptWindow->move(25,25);
    connect(ScriptWindow, &AScriptWindow::WindowShown, WindowNavigator, &WindowNavigatorClass::ShowWindowTriggered);
    connect(ScriptWindow, &AScriptWindow::WindowHidden, WindowNavigator, &WindowNavigatorClass::HideWindowTriggered);
    connect(SM, &AScriptManager::reportProgress, WindowNavigator, &WindowNavigatorClass::setProgress);
    NetModule->SetScriptManager(SM);

    // interface objects are owned after this by the ScriptManager!

    ScriptWindow->SetInterfaceObject(0); //initialization

    AInterfaceToMultiThread* threads = new AInterfaceToMultiThread(SM);
    ScriptWindow->SetInterfaceObject(threads, "threads");

    AInterfaceToConfig* conf = new AInterfaceToConfig(Config);
    QObject::connect(conf, SIGNAL(requestReadRasterGeometry()), GeometryWindow, SLOT(readRasterWindowProperties()));
    ScriptWindow->SetInterfaceObject(conf, "config");

    InterfaceToAddObjScript* geo = new InterfaceToAddObjScript(Detector);
    connect(geo, SIGNAL(requestShowCheckUpWindow()), CheckUpWindow, SLOT(showNormal()));
    ScriptWindow->SetInterfaceObject(geo, "geo");

    AInterfaceToMinimizerJavaScript* mini = new AInterfaceToMinimizerJavaScript(SM);
    ScriptWindow->SetInterfaceObject(mini, "mini");  //mini should be before sim to handle abort correctly

    AInterfaceToData* dat = new AInterfaceToData(Config, EventsDataHub);
    QObject::connect(dat, SIGNAL(RequestEventsGuiUpdate()), Rwindow, SLOT(onRequestEventsGuiUpdate()));
    ScriptWindow->SetInterfaceObject(dat, "events");

    InterfaceToSim* sim = new InterfaceToSim(SimulationManager, EventsDataHub, Config, GlobSet->RecNumTreads);
    QObject::connect(sim, SIGNAL(requestStopSimulation()), SimulationManager, SLOT(StopSimulation()));
    ScriptWindow->SetInterfaceObject(sim, "sim");

    InterfaceToReconstructor* rec = new InterfaceToReconstructor(ReconstructionManager, Config, EventsDataHub, TmpHub, GlobSet->RecNumTreads);
    QObject::connect(rec, SIGNAL(RequestStopReconstruction()), ReconstructionManager, SLOT(requestStop()));
    QObject::connect(rec, SIGNAL(RequestUpdateGuiForManifest()), Rwindow, SLOT(onManifestItemsGuiUpdate()));
    ScriptWindow->SetInterfaceObject(rec, "rec");

    AInterfaceToLRF* lrf = new AInterfaceToLRF(Config, EventsDataHub);
    ScriptWindow->SetInterfaceObject(lrf, "lrf");
    ALrfScriptInterface* newLrf = new ALrfScriptInterface(Detector, EventsDataHub);
    ScriptWindow->SetInterfaceObject(newLrf, "newLrf");

    AInterfaceToPMs* pmS = new AInterfaceToPMs(Config);
    ScriptWindow->SetInterfaceObject(pmS, "pms");

    AInterfaceToGraph* graph = new AInterfaceToGraph(TmpHub);
    ScriptWindow->SetInterfaceObject(graph, "graph");

    AInterfaceToHist* hist = new AInterfaceToHist(TmpHub);
    ScriptWindow->SetInterfaceObject(hist, "hist");

    AInterfaceToTTree* tree = new AInterfaceToTTree(TmpHub);
    ScriptWindow->SetInterfaceObject(tree, "tree");
    connect(tree, &AInterfaceToTTree::RequestTreeDraw, GraphWindow, &GraphWindowClass::DrawTree);

    AInterfaceToMessageWindow* txt = new AInterfaceToMessageWindow(SM, ScriptWindow);
    ScriptWindow->SetInterfaceObject(txt, "msg");

    AInterfaceToWebSocket* web = new AInterfaceToWebSocket();
    ScriptWindow->SetInterfaceObject(web, "web");

    AInterfaceToPhotonScript* photon = new AInterfaceToPhotonScript(Config, EventsDataHub);
    ScriptWindow->SetInterfaceObject(photon, "photon");

#ifdef ANTS_FLANN
    AInterfaceToKnnScript* knn = new AInterfaceToKnnScript(ReconstructionManager->KNNmodule);
    ScriptWindow->SetInterfaceObject(knn, "knn");
#endif

#ifdef ANTS_FANN
    AInterfaceToANNScript* ann = new AInterfaceToANNScript();
    ScriptWindow->SetInterfaceObject(ann, "ann");
#endif

    // Interfaces which rely on MainWindow

    InterfaceToGeoWin* geowin = new InterfaceToGeoWin(this, TmpHub);
    ScriptWindow->SetInterfaceObject(geowin, "geowin");

    InterfaceToGraphWin* grwin = new InterfaceToGraphWin(this);
    ScriptWindow->SetInterfaceObject(grwin, "grwin");

    AInterfaceToGuiScript* gui = new AInterfaceToGuiScript(SM);
    ScriptWindow->SetInterfaceObject(gui, "gui");

    AInterfaceToDepoScript* depo = new AInterfaceToDepoScript(this, EventsDataHub);
    ScriptWindow->SetInterfaceObject(depo, "depo"); 

    AInterfaceToOutputWin* out = new AInterfaceToOutputWin(this);
    ScriptWindow->SetInterfaceObject(out, "outwin");

    ScriptWindow->SetShowEvaluationResult(true);

    QObject::connect(ScriptWindow, SIGNAL(onStart()), this, SLOT(onGlobalScriptStarted()));
    QObject::connect(ScriptWindow, SIGNAL(success(QString)), this, SLOT(onGlobalScriptFinished()));
    QObject::connect(ScriptWindow, SIGNAL(RequestDraw(TObject*,QString,bool)), GraphWindow, SLOT(DrawStrOpt(TObject*,QString,bool)));

    ScriptWindow->UpdateHighlight();
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
