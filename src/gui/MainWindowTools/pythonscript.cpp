#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "apythonscriptmanager.h"
#include "detectorclass.h"
#include "eventsdataclass.h"
#include "aglobalsettings.h"
#include "interfacetoglobscript.h"
#include "ainterfacetomessagewindow.h"
#include "scriptminimizer.h"
#include "histgraphinterfaces.h"
#include "ainterfacetoaddobjscript.h"
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
#include "ainterfacetottree.h"
#include "aparticletrackinghistoryinterface.h"

#ifdef ANTS_FLANN
  #include "ainterfacetoknnscript.h"
#endif

#ifdef ANTS_FANN
  #include "ainterfacetoannscript.h"
#endif

void MainWindow::createPythonScriptWindow()
{
  QWidget* w = new QWidget();
  APythonScriptManager* PSM = new APythonScriptManager(Detector->RandGen);
  PythonScriptWindow = new AScriptWindow(PSM, GlobSet, false, w);
  PythonScriptWindow->move(25,25);
  connect(PythonScriptWindow, SIGNAL(WindowShown(QString)), WindowNavigator, SLOT(ShowWindowTriggered(QString)));
  connect(PythonScriptWindow, SIGNAL(WindowHidden(QString)), WindowNavigator, SLOT(HideWindowTriggered(QString)));

  // interface objects are owned after this by the ScriptManager!
  PythonScriptWindow->RegisterCoreInterfaces();

  AInterfaceToConfig* conf = new AInterfaceToConfig(Config);
  QObject::connect(conf, SIGNAL(requestReadRasterGeometry()), GeometryWindow, SLOT(readRasterWindowProperties()));
  PythonScriptWindow->RegisterInterface(conf, "config");

  AInterfaceToAddObjScript* geo = new AInterfaceToAddObjScript(Detector);
  connect(geo, SIGNAL(requestShowCheckUpWindow()), CheckUpWindow, SLOT(showNormal()));
  PythonScriptWindow->RegisterInterface(geo, "geo");

  AInterfaceToMinimizerPythonScript* mini = new AInterfaceToMinimizerPythonScript(PSM);
  PythonScriptWindow->RegisterInterface(mini, "mini");  //mini should be before sim to handle abort correctly

  AInterfaceToData* dat = new AInterfaceToData(Config, EventsDataHub);
  QObject::connect(dat, SIGNAL(RequestEventsGuiUpdate()), Rwindow, SLOT(onRequestEventsGuiUpdate()));
  PythonScriptWindow->RegisterInterface(dat, "events");

  InterfaceToSim* sim = new InterfaceToSim(SimulationManager, EventsDataHub, Config, GlobSet.RecNumTreads);
  QObject::connect(sim, SIGNAL(requestStopSimulation()), SimulationManager, SLOT(StopSimulation()));
  PythonScriptWindow->RegisterInterface(sim, "sim");

  InterfaceToReconstructor* rec = new InterfaceToReconstructor(ReconstructionManager, Config, EventsDataHub, TmpHub, GlobSet.RecNumTreads);
  QObject::connect(rec, SIGNAL(RequestStopReconstruction()), ReconstructionManager, SLOT(requestStop()));
  QObject::connect(rec, SIGNAL(RequestUpdateGuiForManifest()), Rwindow, SLOT(onManifestItemsGuiUpdate()));
  PythonScriptWindow->RegisterInterface(rec, "rec");

  AInterfaceToLRF* lrf = new AInterfaceToLRF(Config, EventsDataHub);
  PythonScriptWindow->RegisterInterface(lrf, "lrf");
  ALrfScriptInterface* newLrf = new ALrfScriptInterface(Detector, EventsDataHub);
  PythonScriptWindow->RegisterInterface(newLrf, "newLrf");

  AInterfaceToPMs* pmS = new AInterfaceToPMs(Config);
  PythonScriptWindow->RegisterInterface(pmS, "pms");

  AInterfaceToGraph* graph = new AInterfaceToGraph(TmpHub);
  PythonScriptWindow->RegisterInterface(graph, "graph");

  AInterfaceToHist* hist = new AInterfaceToHist(TmpHub);
  PythonScriptWindow->RegisterInterface(hist, "hist");

  AInterfaceToTTree* tree = new AInterfaceToTTree(TmpHub);
  PythonScriptWindow->RegisterInterface(tree, "tree");

  AInterfaceToMessageWindow* txt = new AInterfaceToMessageWindow(PSM, PythonScriptWindow);
  PythonScriptWindow->RegisterInterface(txt, "msg");

  AInterfaceToWebSocket* web = new AInterfaceToWebSocket(EventsDataHub);
  QObject::connect(web, &AInterfaceToWebSocket::showTextOnMessageWindow, txt, &AInterfaceToMessageWindow::Append); // make sure this line is after AInterfaceToMessageWindow init
  QObject::connect(web, &AInterfaceToWebSocket::clearTextOnMessageWindow, txt, &AInterfaceToMessageWindow::Clear); // make sure this line is after AInterfaceToMessageWindow init
  PythonScriptWindow->RegisterInterface(web, "web");

  AInterfaceToPhotonScript* photon = new AInterfaceToPhotonScript(Config, EventsDataHub);
  PythonScriptWindow->RegisterInterface(photon, "photon");

  AInterfaceToDepoScript* depo = new AInterfaceToDepoScript(Detector, GlobSet, EventsDataHub);
  PythonScriptWindow->RegisterInterface(depo, "depo");

  AParticleTrackingHistoryInterface* pth = new AParticleTrackingHistoryInterface(*EventsDataHub);
  ScriptWindow->RegisterInterface(pth, "tracklog");

#ifdef ANTS_FLANN
  AInterfaceToKnnScript* knn = new AInterfaceToKnnScript(ReconstructionManager->KNNmodule);
  PythonScriptWindow->RegisterInterface(knn, "knn");
#endif

#ifdef ANTS_FANN
  //AInterfaceToANNScript* ann = new AInterfaceToANNScript();
  //PythonScriptWindow->RegisterInterface(ann, "ann");
#endif

  // Interfaces which rely on MainWindow

  InterfaceToGeoWin* geowin = new InterfaceToGeoWin(this, TmpHub);
  PythonScriptWindow->RegisterInterface(geowin, "geowin");

  InterfaceToGraphWin* grwin = new InterfaceToGraphWin(this);
  PythonScriptWindow->RegisterInterface(grwin, "grwin");

  AInterfaceToOutputWin* out = new AInterfaceToOutputWin(this);
  PythonScriptWindow->RegisterInterface(out, "outwin");

  PythonScriptWindow->SetShowEvaluationResult(true);

  QObject::connect(PythonScriptWindow, SIGNAL(onStart()), this, SLOT(onGlobalScriptStarted()));
  QObject::connect(PythonScriptWindow, SIGNAL(success(QString)), this, SLOT(onGlobalScriptFinished()));
  QObject::connect(PythonScriptWindow, SIGNAL(RequestDraw(TObject*,QString,bool)), GraphWindow, SLOT(DrawStrOpt(TObject*,QString,bool)));

  PythonScriptWindow->UpdateGui();
}
