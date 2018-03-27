#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "apythonscriptmanager.h"
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

void MainWindow::createPythonScriptWindow()
{
  QWidget* w = new QWidget();
  APythonScriptManager* PSM = new APythonScriptManager(Detector->RandGen);
  PythonScriptWindow = new AScriptWindow(PSM, GlobSet, w);
  PythonScriptWindow->move(25,25);
  connect(PythonScriptWindow, SIGNAL(WindowShown(QString)), WindowNavigator, SLOT(ShowWindowTriggered(QString)));
  connect(PythonScriptWindow, SIGNAL(WindowHidden(QString)), WindowNavigator, SLOT(HideWindowTriggered(QString)));

  // interface objects are owned after this by the ScriptManager!
  PythonScriptWindow->SetInterfaceObject(0); //initialization

  AInterfaceToConfig* conf = new AInterfaceToConfig(Config);
  QObject::connect(conf, SIGNAL(requestReadRasterGeometry()), GeometryWindow, SLOT(readRasterWindowProperties()));
  PythonScriptWindow->SetInterfaceObject(conf, "config");

  InterfaceToAddObjScript* geo = new InterfaceToAddObjScript(Detector);
  connect(geo, SIGNAL(requestShowCheckUpWindow()), CheckUpWindow, SLOT(showNormal()));
  PythonScriptWindow->SetInterfaceObject(geo, "geo");

/*
  AInterfaceToMinimizerScript* mini = new AInterfaceToMinimizerScript(ScriptWindow->ScriptManager);
  PythonScriptWindow->SetInterfaceObject(mini, "mini");  //mini should be before sim to handle abort correctly
*/

  AInterfaceToData* dat = new AInterfaceToData(Config, EventsDataHub);
  QObject::connect(dat, SIGNAL(RequestEventsGuiUpdate()), Rwindow, SLOT(onRequestEventsGuiUpdate()));
  PythonScriptWindow->SetInterfaceObject(dat, "events");

  InterfaceToSim* sim = new InterfaceToSim(SimulationManager, EventsDataHub, Config, GlobSet->RecNumTreads);
  QObject::connect(sim, SIGNAL(requestStopSimulation()), SimulationManager, SLOT(StopSimulation()));
  PythonScriptWindow->SetInterfaceObject(sim, "sim");

  InterfaceToReconstructor* rec = new InterfaceToReconstructor(ReconstructionManager, Config, EventsDataHub, TmpHub, GlobSet->RecNumTreads);
  QObject::connect(rec, SIGNAL(RequestStopReconstruction()), ReconstructionManager, SLOT(requestStop()));
  QObject::connect(rec, SIGNAL(RequestUpdateGuiForManifest()), Rwindow, SLOT(onManifestItemsGuiUpdate()));
  PythonScriptWindow->SetInterfaceObject(rec, "rec");

  AInterfaceToLRF* lrf = new AInterfaceToLRF(Config, EventsDataHub);
  PythonScriptWindow->SetInterfaceObject(lrf, "lrf");
  ALrfScriptInterface* newLrf = new ALrfScriptInterface(Detector, EventsDataHub);
  PythonScriptWindow->SetInterfaceObject(newLrf, "newLrf");

  AInterfaceToPMs* pmS = new AInterfaceToPMs(Config);
  PythonScriptWindow->SetInterfaceObject(pmS, "pms");

  AInterfaceToGraph* graph = new AInterfaceToGraph(TmpHub);
  PythonScriptWindow->SetInterfaceObject(graph, "graph");

  AInterfaceToHist* hist = new AInterfaceToHist(TmpHub);
  PythonScriptWindow->SetInterfaceObject(hist, "hist");

  AInterfaceToTree* tree = new AInterfaceToTree(TmpHub);
  PythonScriptWindow->SetInterfaceObject(tree, "tree");

  AInterfaceToMessageWindow* txt = new AInterfaceToMessageWindow(PSM, PythonScriptWindow);
  PythonScriptWindow->SetInterfaceObject(txt, "msg");

  AInterfaceToWebSocket* web = new AInterfaceToWebSocket();
  PythonScriptWindow->SetInterfaceObject(web, "web");

  AInterfaceToPhotonScript* photon = new AInterfaceToPhotonScript(Config, EventsDataHub);
  PythonScriptWindow->SetInterfaceObject(photon, "photon");

  /*
#ifdef ANTS_FLANN
  AInterfaceToKnnScript* knn = new AInterfaceToKnnScript(ReconstructionManager->KNNmodule);
  PythonScriptWindow->SetInterfaceObject(knn, "knn");
#endif

#ifdef ANTS_FANN
  AInterfaceToANNScript* ann = new AInterfaceToANNScript();
  PythonScriptWindow->SetInterfaceObject(ann, "ann");
#endif
  */

  // Interfaces which rely on MainWindow

  InterfaceToGeoWin* geowin = new InterfaceToGeoWin(this, TmpHub);
  PythonScriptWindow->SetInterfaceObject(geowin, "geowin");

  InterfaceToGraphWin* grwin = new InterfaceToGraphWin(this);
  PythonScriptWindow->SetInterfaceObject(grwin, "grwin");

  AInterfaceToDepoScript* depo = new AInterfaceToDepoScript(this, EventsDataHub);
  PythonScriptWindow->SetInterfaceObject(depo, "depo");

  AInterfaceToOutputWin* out = new AInterfaceToOutputWin(this);
  PythonScriptWindow->SetInterfaceObject(out, "outwin");

  PythonScriptWindow->SetShowEvaluationResult(true);

  QObject::connect(PythonScriptWindow, SIGNAL(onStart()), this, SLOT(onGlobalScriptStarted()));
  QObject::connect(PythonScriptWindow, SIGNAL(success(QString)), this, SLOT(onGlobalScriptFinished()));
  QObject::connect(PythonScriptWindow, SIGNAL(RequestDraw(TObject*,QString,bool)), GraphWindow, SLOT(DrawStrOpt(TObject*,QString,bool)));

  PythonScriptWindow->UpdateHighlight();
}
