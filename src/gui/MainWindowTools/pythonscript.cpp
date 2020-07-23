#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "apythonscriptmanager.h"
#include "detectorclass.h"
#include "eventsdataclass.h"
#include "aglobalsettings.h"
#include "amsg_si.h"
#include "scriptminimizer.h"
#include "histgraphinterfaces.h"
#include "ageo_si.h"
#include "graphwindowclass.h"
#include "geometrywindowclass.h"
#include "aconfiguration.h"
#include "reconstructionwindow.h"
#include "areconstructionmanager.h"
#include "windownavigatorclass.h"
#include "asimulationmanager.h"
#include "lrfwindow.h"
#include "ascriptwindow.h"
#include "checkupwindowclass.h"
#include "aweb_si.h"
#include "anetworkmodule.h"
#include "aphoton_si.h"
#include "athreads_si.h"
#include "atree_si.h"
#include "asim_si.h"
#include "apthistory_si.h"
#include "aconfig_si.h"
#include "aevents_si.h"
#include "arec_si.h"
#include "apms_si.h"
#include "alrf_si.h"
#include "ageowin_si.h"
#include "agraphwin_si.h"
#include "aoutwin_si.h"

#ifdef ANTS_FLANN
  #include "aknn_si.h"
#endif

#ifdef ANTS_FANN
  #include "aann_si.h"
#endif

void MainWindow::createPythonScriptWindow()
{
  QWidget* w = new QWidget();
  APythonScriptManager* PSM = new APythonScriptManager(Detector->RandGen);
  PythonScriptWindow = new AScriptWindow(PSM, false, w);
  PythonScriptWindow->move(25,25);
  connect(PythonScriptWindow, SIGNAL(WindowShown(QString)), WindowNavigator, SLOT(ShowWindowTriggered(QString)));
  connect(PythonScriptWindow, SIGNAL(WindowHidden(QString)), WindowNavigator, SLOT(HideWindowTriggered(QString)));
  PythonScriptWindow->connectWinNavigator(WindowNavigator);

  // interface objects are owned after this by the ScriptManager!
  PythonScriptWindow->RegisterCoreInterfaces();

  AConfig_SI* conf = new AConfig_SI(Config);
  QObject::connect(conf, SIGNAL(requestReadRasterGeometry()), GeometryWindow, SLOT(readRasterWindowProperties()));
  PythonScriptWindow->RegisterInterface(conf, "config");

  AGeo_SI* geo = new AGeo_SI(Detector);
  connect(geo, SIGNAL(requestShowCheckUpWindow()), CheckUpWindow, SLOT(showNormal()));
  PythonScriptWindow->RegisterInterface(geo, "geo");

  AMini_Python_SI* mini = new AMini_Python_SI(PSM);
  PythonScriptWindow->RegisterInterface(mini, "mini");  //mini should be before sim to handle abort correctly

  AEvents_SI* dat = new AEvents_SI(Config, EventsDataHub);
  QObject::connect(dat, SIGNAL(RequestEventsGuiUpdate()), Rwindow, SLOT(onRequestEventsGuiUpdate()));
  PythonScriptWindow->RegisterInterface(dat, "events");

  ASim_SI* sim = new ASim_SI(SimulationManager, EventsDataHub, Config);
  QObject::connect(sim, SIGNAL(requestStopSimulation()), SimulationManager, SLOT(StopSimulation()));
  PythonScriptWindow->RegisterInterface(sim, "sim");

  APTHistory_SI * ptHistory = new APTHistory_SI(*SimulationManager);
  ScriptWindow->RegisterInterface(ptHistory, "ptHistory");

  ARec_SI* rec = new ARec_SI(ReconstructionManager, Config, EventsDataHub, TmpHub);
  QObject::connect(rec, SIGNAL(RequestStopReconstruction()), ReconstructionManager, SLOT(requestStop()));
  QObject::connect(rec, SIGNAL(RequestUpdateGuiForManifest()), Rwindow, SLOT(onManifestItemsGuiUpdate()));
  PythonScriptWindow->RegisterInterface(rec, "rec");

  ALrf_SI* lrf = new ALrf_SI(Config, EventsDataHub);
  PythonScriptWindow->RegisterInterface(lrf, "lrf");
  ALrfRaim_SI* newLrf = new ALrfRaim_SI(Detector, EventsDataHub);
  PythonScriptWindow->RegisterInterface(newLrf, "newLrf");

  APms_SI* pmS = new APms_SI(Config);
  PythonScriptWindow->RegisterInterface(pmS, "pms");

  AInterfaceToGraph* graph = new AInterfaceToGraph(TmpHub);
  PythonScriptWindow->RegisterInterface(graph, "graph");

  AInterfaceToHist* hist = new AInterfaceToHist(TmpHub);
  PythonScriptWindow->RegisterInterface(hist, "hist");

  ATree_SI* tree = new ATree_SI(TmpHub);
  PythonScriptWindow->RegisterInterface(tree, "tree");
  connect(tree, &ATree_SI::RequestTreeDraw, GraphWindow, &GraphWindowClass::DrawTree);

  AMsg_SI* txt = new AMsg_SI(PSM, PythonScriptWindow);
  PythonScriptWindow->RegisterInterface(txt, "msg");

  AWeb_SI* web = new AWeb_SI(EventsDataHub);
  QObject::connect(web, &AWeb_SI::showTextOnMessageWindow, txt, &AMsg_SI::Append); // make sure this line is after AInterfaceToMessageWindow init
  QObject::connect(web, &AWeb_SI::clearTextOnMessageWindow, txt, &AMsg_SI::Clear); // make sure this line is after AInterfaceToMessageWindow init
  PythonScriptWindow->RegisterInterface(web, "web");

  APhoton_SI* photon = new APhoton_SI(Config, EventsDataHub, *SimulationManager);
  PythonScriptWindow->RegisterInterface(photon, "photon");

#ifdef ANTS_FLANN
  AKnn_SI* knn = new AKnn_SI(ReconstructionManager->KNNmodule);
  PythonScriptWindow->RegisterInterface(knn, "knn");
#endif

#ifdef ANTS_FANN
  //AAnn_SI* ann = new AAnn_SI();
  //PythonScriptWindow->RegisterInterface(ann, "ann");
#endif

  // Interfaces which rely on MainWindow

  AGeoWin_SI* geowin = new AGeoWin_SI(this, SimulationManager);
  PythonScriptWindow->RegisterInterface(geowin, "geowin");

  AGraphWin_SI* grwin = new AGraphWin_SI(this);
  PythonScriptWindow->RegisterInterface(grwin, "grwin");

  AOutWin_SI* out = new AOutWin_SI(this);
  PythonScriptWindow->RegisterInterface(out, "outwin");

  QObject::connect(PythonScriptWindow, SIGNAL(onStart()), this, SLOT(onGlobalScriptStarted()));
  QObject::connect(PythonScriptWindow, SIGNAL(success(QString)), this, SLOT(onGlobalScriptFinished()));
  QObject::connect(PythonScriptWindow, SIGNAL(RequestDraw(TObject*,QString,bool)), GraphWindow, SLOT(DrawStrOpt(TObject*,QString,bool)));

  PythonScriptWindow->UpdateGui();
}
