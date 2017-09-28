//ANTS2

#include "detectorclass.h"
#include "eventsdataclass.h"
#include "reconstructionmanagerclass.h"
#include "amaterialparticlecolection.h"
#include "tmpobjhubclass.h"
#include "aconfiguration.h"
#include "ascriptmanager.h"
#include "interfacetoglobscript.h"
#include "scriptminimizer.h"
#include "globalsettingsclass.h"
#include "afiletools.h"
#include "aqtmessageredirector.h"
#include "particlesourcesclass.h"
#include "anetworkmodule.h"
#include "asandwich.h"

// SIM
#ifdef SIM
#include "simulationmanager.h"
#endif

// GUI
#ifdef GUI
#include "mainwindow.h"
#include "exampleswindow.h"
#include "genericscriptwindowclass.h"
#endif

//Qt
#include <QApplication>
#include <QLocale>
#include <QDebug>
#include <QThread>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QLoggingCategory>

//Root
#include "TApplication.h"
#include "TObject.h"
#include "TH1.h"
#include "RVersion.h"
#if ROOT_VERSION_CODE < ROOT_VERSION(6,11,1)
#include "TThread.h"
#endif

#include "amessage.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QLocale::setDefault(QLocale("en_US"));

    AConfiguration Config;
    int rootargc=1;
    char *rootargv[] = {(char*)"qqq"};
    TApplication RootApp("My ROOT", &rootargc, rootargv);
    //qDebug() << "___> Root App created";
#if ROOT_VERSION_CODE < ROOT_VERSION(6,11,1)
    TThread::Initialize();
    //qDebug() << "___> TThread initialized";
#endif
    EventsDataClass EventsDataHub;

    //qDebug() << "___> EventsDataHub created";
    DetectorClass Detector(&Config);
    Config.SetDetector(&Detector);
    QObject::connect(Detector.MpCollection, &AMaterialParticleCollection::ParticleCollectionChanged, &Config, &AConfiguration::UpdateParticlesJson);
    QObject::connect(&Detector, &DetectorClass::requestClearEventsData, &EventsDataHub, &EventsDataClass::clear);
    //qDebug() << "___> Detector created";

#ifdef SIM
    ASimulationManager SimulationManager(&EventsDataHub, &Detector);
    Config.SetParticleSources(SimulationManager.ParticleSources);
    //qDebug() << "___> Simulation manager created";
#endif

    ReconstructionManagerClass ReconstructionManager(&EventsDataHub, Detector.PMs, Detector.PMgroups, Detector.LRFs, &Detector.GeoManager);
    //qDebug() << "___> Reconstruction manager created";

    TmpObjHubClass TmpHub;
    QObject::connect(&EventsDataHub, &EventsDataClass::cleared, &TmpHub, &TmpObjHubClass::Clear);
    //qDebug() << "___> Tmp objects hub created";

    ANetworkModule Network;
    QObject::connect(&Detector, &DetectorClass::newGeoManager, &Network, &ANetworkModule::onNewGeoManagerCreated);
    QObject::connect(&Network, &ANetworkModule::RootServerStarted, &Detector, &DetectorClass::onRequestRegisterGeoManager);
      //in GlobSetWindow init now:
      //Network.StartRootHttpServer();  //does nothing if compilation flag is not set
      //Network.StartWebSocketServer(1234);
    //qDebug() << "___> Network module created";

    GlobalSettingsClass GlobSet(&Network);
    if (GlobSet.NumThreads == -1) GlobSet.NumThreads = GlobSet.RecNumTreads;
    //qDebug() << "___> Global settings object created";

    Config.UpdateLRFmakeJson(); //compatibility    
    TH1::AddDirectory(false);  //a histograms objects will not be automatically created in root directory (TDirectory); special case is in TreeView and ResolutionVsArea
                               // see ReconstructionWindow::on_pbTreeView_clicked()

    //SUPPRESS WARNINGS about ssl - only needed it on MSVC2012
    QLoggingCategory::setFilterRules("qt.network.ssl.warning=false");

    //qDebug() << "___> Selecting application type";
#ifdef GUI
    if(argc == 1)
    {
        //GUI application
        //qDebug() << "___> Creating MainWindow";
        MainWindow w(&Detector, &EventsDataHub, &RootApp, &SimulationManager, &ReconstructionManager, &Network, &TmpHub, &GlobSet);  //Network - still need to set script manager!
        //qDebug() << "___> Showing MainWindow...";
        w.show();
        //qDebug() << "___> Done!";

        //overrides the saved status of examples window
        if (GlobSet.ShowExamplesOnStart)
        {
            w.ELwindow->show();
            w.ELwindow->raise();//making sure examples window is on top
        }
        else w.ELwindow->hide();
        //qDebug() << "___> Examples window shown/hidden";

        //qDebug() << "___> All done: starting application.";
        return a.exec();
    }
    else if (argc == 2 && (QString(argv[1])=="-b" || QString(argv[1])=="--batch") )
    {
        GenericScriptWindowClass GenScriptWindow(Detector.RandGen);

        InterfaceToGlobScript* interObj = new InterfaceToGlobScript();
        GenScriptWindow.SetInterfaceObject(interObj); // dummy interface for now, just used to identify "Global script" mode

        InterfaceToConfig* conf = new InterfaceToConfig(&Config);
        GenScriptWindow.SetInterfaceObject(conf, "config");

        InterfaceToAddObjScript* geo = new InterfaceToAddObjScript(&Detector);
        GenScriptWindow.SetInterfaceObject(geo, "geo");

        InterfaceToMinimizerScript* mini = new InterfaceToMinimizerScript(GenScriptWindow.ScriptManager);
        GenScriptWindow.SetInterfaceObject(mini, "mini");  //mini should be before sim to handle abort correctly

        InterfaceToData* dat = new InterfaceToData(&Config, &ReconstructionManager, &EventsDataHub);
        GenScriptWindow.SetInterfaceObject(dat, "events");

        InterfaceToSim* sim = new InterfaceToSim(&SimulationManager, &EventsDataHub, &Config, GlobSet.RecNumTreads, false);
        QObject::connect(sim, SIGNAL(requestStopSimulation()), &SimulationManager, SLOT(StopSimulation()));
        GenScriptWindow.SetInterfaceObject(sim, "sim");

        InterfaceToReconstructor* rec = new InterfaceToReconstructor(&ReconstructionManager, &Config, &EventsDataHub, GlobSet.RecNumTreads);
        QObject::connect(rec, SIGNAL(RequestStopReconstruction()), &ReconstructionManager, SLOT(requestStop()));
        GenScriptWindow.SetInterfaceObject(rec, "rec");

        InterfaceToLRF* lrf = new InterfaceToLRF(&Config, &EventsDataHub);
        GenScriptWindow.SetInterfaceObject(lrf, "lrf");
        ALrfScriptInterface* newLrf = new ALrfScriptInterface(&Detector, &EventsDataHub);
        GenScriptWindow.SetInterfaceObject(newLrf, "newLrf");

        AInterfaceToPMs* pmS = new AInterfaceToPMs(&Config);
        GenScriptWindow.SetInterfaceObject(pmS, "pms");

        InterfaceToGraphs* graph = new InterfaceToGraphs(&TmpHub);
        GenScriptWindow.SetInterfaceObject(graph, "graph");

        InterfaceToHistD* hist = new InterfaceToHistD(&TmpHub);
        GenScriptWindow.SetInterfaceObject(hist, "hist");

        AInterfaceToTree* tree = new AInterfaceToTree(&TmpHub);
        GenScriptWindow.SetInterfaceObject(tree, "tree");

        InterfaceToTexter* txt = new InterfaceToTexter(&GenScriptWindow);
        GenScriptWindow.SetInterfaceObject(txt, "msg");

        //Setting up the window
        GenScriptWindow.SetStarterDir(GlobSet.LibScripts, GlobSet.LastOpenDir, GlobSet.ExamplesDir);
        GenScriptWindow.SetExample(""); //empty example will force to start example explorer on "Example" button pressed
        GenScriptWindow.SetShowEvaluationResult(true);
        GenScriptWindow.SetTitle("ANTS2 batch mode");
        GenScriptWindow.SetScript(&GlobSet.GlobScript);

        QObject::connect(&GenScriptWindow, SIGNAL(success(QString)), &GenScriptWindow, SLOT(updateJsonTree()));

        if (QFile(GlobSet.ExamplesDir + "/StartupDetector.json").exists())
            Config.LoadConfig(GlobSet.ExamplesDir + "/StartupDetector.json");
        else
            message("Default detector configuration not loaded - file not found");

        GenScriptWindow.SetJsonTreeAlwaysVisible(true);
        GenScriptWindow.show();
        return a.exec();
    }
    else
#else // GUI
    if (argc > 1)
#endif // GUI
    {    
        //direct script mode
        QString fileName = QString(argv[1]);
        //QString fileName = "/home/vova/Work/GAMMA/ANTS2V3/myscript.js";
        if (!QFile(fileName).exists())
        {
            qDebug() << "File not found:"<<fileName;
            return -101;
        }
        QString script;
        bool ok = LoadTextFromFile(fileName, script);
        if (!ok)
        {
            qDebug() << "Failed to read script from file:"<<fileName;
            return -102;
        }

        QString path = QFileInfo(fileName).absolutePath();
        QDir::setCurrent(path);

        if (argc == 4 && (QString(argv[2])=="-o" || QString(argv[2])=="--output") )
        {
            QString LogFileName = QString(argv[3]);
            AQtMessageRedirector rd; //redirecting qDebug, qWarning etc to file
            if (!rd.activate(LogFileName))
            {
                qDebug() << "Failed to redirect output to file:"<<LogFileName;
                return -103;
            }
        }

        qDebug() << "Script from file:"<<fileName;

        AScriptManager SM(Detector.RandGen);
        InterfaceToGlobScript* interObj = new InterfaceToGlobScript();
        SM.SetInterfaceObject(interObj); // dummy interface for now, just used to identify "Global script" mode
        InterfaceToConfig* conf = new InterfaceToConfig(&Config);
        SM.SetInterfaceObject(conf, "config");
        InterfaceToAddObjScript* geo = new InterfaceToAddObjScript(&Detector);
        SM.SetInterfaceObject(geo, "geo");
        InterfaceToMinimizerScript* mini = new InterfaceToMinimizerScript(&SM);
        SM.SetInterfaceObject(mini, "mini");  //mini should be before sim to handle abort correctly
        InterfaceToData* dat = new InterfaceToData(&Config, &ReconstructionManager, &EventsDataHub);
        SM.SetInterfaceObject(dat, "events");
#ifdef SIM
        InterfaceToSim* sim = new InterfaceToSim(&SimulationManager, &EventsDataHub, &Config, GlobSet.RecNumTreads, false);
        SM.SetInterfaceObject(sim, "sim");
#endif
        InterfaceToReconstructor* rec = new InterfaceToReconstructor(&ReconstructionManager, &Config, &EventsDataHub, GlobSet.RecNumTreads);
        SM.SetInterfaceObject(rec, "rec");
        InterfaceToLRF* lrf = new InterfaceToLRF(&Config, &EventsDataHub);
        SM.SetInterfaceObject(lrf, "lrf");
        ALrfScriptInterface* newLrf = new ALrfScriptInterface(&Detector, &EventsDataHub);
        SM.SetInterfaceObject(newLrf, "newLrf");
        AInterfaceToPMs* pmS = new AInterfaceToPMs(&Config);
        SM.SetInterfaceObject(pmS, "pms");
        InterfaceToGraphs* graph = new InterfaceToGraphs(&TmpHub);
        SM.SetInterfaceObject(graph, "graph");
        InterfaceToHistD* hist = new InterfaceToHistD(&TmpHub);
        SM.SetInterfaceObject(hist, "hist");
        AInterfaceToTree* tree = new AInterfaceToTree(&TmpHub);
        SM.SetInterfaceObject(tree, "tree");

        int errorLineNum = SM.FindSyntaxError(script); //qDebug is already inside
        if (errorLineNum > -1)
            return 0;

        QString result = SM.Evaluate(script);
        qDebug() << "Script evaluation result:"<<result;        
        return 0;
    }

    Network.StopRootHttpServer();
    Network.StopWebSocketServer();
}

