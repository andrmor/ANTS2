#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "genericscriptwindowclass.h"
#include "detectorclass.h"
#include "eventsdataclass.h"
#include "globalsettingsclass.h"
#include "interfacetoglobscript.h"
#include "scriptminimizer.h"
#include "scriptinterfaces.h"
#include "interfacetocheckerscript.h"
#include "graphwindowclass.h"
#include "geometrywindowclass.h"
#include "aconfiguration.h"
#include "reconstructionwindow.h"
#include "reconstructionmanagerclass.h"
#include "windownavigatorclass.h"
#include "simulationmanager.h"
#include "lrfwindow.h"
#include "ascriptwindow.h"
#include "checkupwindowclass.h"
#include "ainterfacetowebsocket.h"
#include "anetworkmodule.h"
#include "ainterfacetophotonscript.h"

#ifdef ANTS_FLANN
  #include "ainterfacetoknnscript.h"
#endif

#include <QDebug>

void MainWindow::createScriptWindow()
{
    QWidget* w = new QWidget();
    ScriptWindow = new AScriptWindow(GlobSet, Detector->RandGen, w);
    ScriptWindow->move(25,25);
    connect(ScriptWindow, SIGNAL(WindowShown(QString)), WindowNavigator, SLOT(ShowWindowTriggered(QString)));
    connect(ScriptWindow, SIGNAL(WindowHidden(QString)), WindowNavigator, SLOT(HideWindowTriggered(QString)));
    NetModule->SetScriptManager(ScriptWindow->ScriptManager);

    // interface objects are owned after this by the ScriptManager!

    InterfaceToGlobScript* interObj = new InterfaceToGlobScript();
    ScriptWindow->SetInterfaceObject(interObj); // dummy interface for now, just used to identify "Global script" mode

    InterfaceToConfig* conf = new InterfaceToConfig(Config);
    QObject::connect(conf, SIGNAL(requestReadRasterGeometry()), GeometryWindow, SLOT(readRasterWindowProperties()));
    ScriptWindow->SetInterfaceObject(conf, "config");

    InterfaceToAddObjScript* geo = new InterfaceToAddObjScript(Detector);
    connect(geo, SIGNAL(requestShowCheckUpWindow()), CheckUpWindow, SLOT(showNormal()));
    ScriptWindow->SetInterfaceObject(geo, "geo");

    InterfaceToMinimizerScript* mini = new InterfaceToMinimizerScript(ScriptWindow->ScriptManager);
    ScriptWindow->SetInterfaceObject(mini, "mini");  //mini should be before sim to handle abort correctly

    InterfaceToData* dat = new InterfaceToData(Config, ReconstructionManager, EventsDataHub);
    QObject::connect(dat, SIGNAL(RequestEventsGuiUpdate()), Rwindow, SLOT(onRequestEventsGuiUpdate()));
    ScriptWindow->SetInterfaceObject(dat, "events");

    InterfaceToSim* sim = new InterfaceToSim(SimulationManager, EventsDataHub, Config, GlobSet->RecNumTreads);
    QObject::connect(sim, SIGNAL(requestStopSimulation()), SimulationManager, SLOT(StopSimulation()));
    ScriptWindow->SetInterfaceObject(sim, "sim");

    InterfaceToReconstructor* rec = new InterfaceToReconstructor(ReconstructionManager, Config, EventsDataHub, GlobSet->RecNumTreads);
    QObject::connect(rec, SIGNAL(RequestStopReconstruction()), ReconstructionManager, SLOT(requestStop()));
    QObject::connect(rec, SIGNAL(RequestUpdateGuiForManifest()), Rwindow, SLOT(onManifestItemsGuiUpdate()));
    ScriptWindow->SetInterfaceObject(rec, "rec");

    InterfaceToLRF* lrf = new InterfaceToLRF(Config, EventsDataHub);
    ScriptWindow->SetInterfaceObject(lrf, "lrf");
    ALrfScriptInterface* newLrf = new ALrfScriptInterface(Detector, EventsDataHub);
    ScriptWindow->SetInterfaceObject(newLrf, "newLrf");

    AInterfaceToPMs* pmS = new AInterfaceToPMs(Config);
    ScriptWindow->SetInterfaceObject(pmS, "pms");

    InterfaceToGraphs* graph = new InterfaceToGraphs(TmpHub);
    ScriptWindow->SetInterfaceObject(graph, "graph");

    InterfaceToHistD* hist = new InterfaceToHistD(TmpHub);
    ScriptWindow->SetInterfaceObject(hist, "hist");

    AInterfaceToTree* tree = new AInterfaceToTree(TmpHub);
    ScriptWindow->SetInterfaceObject(tree, "tree");

    InterfaceToTexter* txt = new InterfaceToTexter(ScriptWindow);
    ScriptWindow->SetInterfaceObject(txt, "msg");

    AInterfaceToWebSocket* web = new AInterfaceToWebSocket();
    ScriptWindow->SetInterfaceObject(web, "web");

    AInterfaceToPhotonScript* photon = new AInterfaceToPhotonScript(Config, EventsDataHub);
    ScriptWindow->SetInterfaceObject(photon, "photon");

#ifdef ANTS_FLANN
    AInterfaceToKnnScript* knn = new AInterfaceToKnnScript(ReconstructionManager->KNNmodule);
    ScriptWindow->SetInterfaceObject(knn, "knn");
#endif

    // Interfaces which rely on MainWindow

    InterfaceToGeoWin* geowin = new InterfaceToGeoWin(this, TmpHub);
    ScriptWindow->SetInterfaceObject(geowin, "geowin");

    InterfaceToGraphWin* grwin = new InterfaceToGraphWin(this);
    ScriptWindow->SetInterfaceObject(grwin, "grwin");

    InterfaceToInteractionScript* depo = new InterfaceToInteractionScript(this, EventsDataHub);
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
