//ANTS2

#include "detectorclass.h"
#include "eventsdataclass.h"
#include "areconstructionmanager.h"
#include "amaterialparticlecolection.h"
#include "tmpobjhubclass.h"
#include "aconfiguration.h"
#include "ascriptmanager.h"
#include "interfacetoglobscript.h"
#include "histgraphinterfaces.h"
#include "scriptminimizer.h"
#include "globalsettingsclass.h"
#include "afiletools.h"
#include "aqtmessageredirector.h"
#include "particlesourcesclass.h"
#include "anetworkmodule.h"
#include "asandwich.h"
#include "amessageoutput.h"

// SIM
#ifdef SIM
#include "simulationmanager.h"
#endif

// GUI
#ifdef GUI
#include "mainwindow.h"
#include "exampleswindow.h"
#include "genericscriptwindowclass.h"
#include "ainterfacetomessagewindow.h"
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
#include <QtMessageHandler>

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

    //setting up logging
    int debug_verbosity = DEBUG_VERBOSITY;
    qDebug() << "Debug verbosity is set to level "<<debug_verbosity;
    QString FilterRules;
    switch (debug_verbosity)
    {
    case 0:
        //QT_LOGGING_RULES = "*.debug=false;";
        FilterRules += "*.debug=false";
        break;
    case 2:
        qInstallMessageHandler(AMessageOutput);
        break;
    case 1:
    default:
        qInstallMessageHandler(0);
    }
    FilterRules += "\nqt.network.ssl.warning=false"; //to suppress warnings about ssl
    //QLoggingCategory::setFilterRules("qt.network.ssl.warning=false");
    QLoggingCategory::setFilterRules(FilterRules);

    AConfiguration Config;
    int rootargc=1;
    char *rootargv[] = {(char*)"qqq"};
    TApplication RootApp("My ROOT", &rootargc, rootargv);
    qDebug() << "Root App created";
#if ROOT_VERSION_CODE < ROOT_VERSION(6,11,1)
    TThread::Initialize();
    qDebug() << "TThread initialized";
#endif
    EventsDataClass EventsDataHub;

    qDebug() << "EventsDataHub created";
    DetectorClass Detector(&Config);
    Config.SetDetector(&Detector);
    QObject::connect(Detector.MpCollection, &AMaterialParticleCollection::ParticleCollectionChanged, &Config, &AConfiguration::UpdateParticlesJson);
    QObject::connect(&Detector, &DetectorClass::requestClearEventsData, &EventsDataHub, &EventsDataClass::clear);
    qDebug() << "Detector created";

#ifdef SIM
    ASimulationManager SimulationManager(&EventsDataHub, &Detector);
    Config.SetParticleSources(SimulationManager.ParticleSources);
    qDebug() << "Simulation manager created";
#endif

    TmpObjHubClass TmpHub;
    QObject::connect(&EventsDataHub, &EventsDataClass::cleared, &TmpHub, &TmpObjHubClass::Clear);
    qDebug() << "Tmp objects hub created";

    AReconstructionManager ReconstructionManager(&EventsDataHub, &Detector, &TmpHub);
    qDebug() << "Reconstruction manager created";

    ANetworkModule Network;
    QObject::connect(&Detector, &DetectorClass::newGeoManager, &Network, &ANetworkModule::onNewGeoManagerCreated);
    QObject::connect(&Network, &ANetworkModule::RootServerStarted, &Detector, &DetectorClass::onRequestRegisterGeoManager);
      //in GlobSetWindow init now:
      //Network.StartRootHttpServer();  //does nothing if compilation flag is not set
      //Network.StartWebSocketServer(1234);
    qDebug() << "Network module created";

    GlobalSettingsClass GlobSet(&Network);
    if (GlobSet.NumThreads == -1) GlobSet.NumThreads = GlobSet.RecNumTreads;
    qDebug() << "Global settings object created";

    Config.UpdateLRFmakeJson(); //compatibility    
    TH1::AddDirectory(false);  //a histograms objects will not be automatically created in root directory (TDirectory); special case is in TreeView and ResolutionVsArea
                               // see ReconstructionWindow::on_pbTreeView_clicked()

    qDebug() << "Selecting application type";
#ifdef GUI
    if(argc == 1)
    {
        //GUI application
        qDebug() << "Creating MainWindow";
        MainWindow w(&Detector, &EventsDataHub, &RootApp, &SimulationManager, &ReconstructionManager, &Network, &TmpHub, &GlobSet);  //Network - still need to set script manager!
        qDebug() << "Showing MainWindow";
        w.show();

        //overrides the saved status of examples window
        if (GlobSet.ShowExamplesOnStart)
        {
            w.ELwindow->show();
            w.ELwindow->raise();//making sure examples window is on top
        }
        else w.ELwindow->hide();
        qDebug() << "Examples window shown/hidden";

        qDebug() << "Starting QApplication";
        return a.exec();
    }
    else if (argc == 2 && (QString(argv[1])=="-b" || QString(argv[1])=="--batch") )
    {
        GenericScriptWindowClass GenScriptWindow(Detector.RandGen);

        GenScriptWindow.SetInterfaceObject(0); //no replacement for the global object in "gloal script" mode

        AInterfaceToConfig* conf = new AInterfaceToConfig(&Config);
        GenScriptWindow.SetInterfaceObject(conf, "config");

        InterfaceToAddObjScript* geo = new InterfaceToAddObjScript(&Detector);
        GenScriptWindow.SetInterfaceObject(geo, "geo");

        AInterfaceToMinimizerScript* mini = new AInterfaceToMinimizerScript(GenScriptWindow.ScriptManager);
        GenScriptWindow.SetInterfaceObject(mini, "mini");  //mini should be before sim to handle abort correctly

        AInterfaceToData* dat = new AInterfaceToData(&Config, &EventsDataHub);
        GenScriptWindow.SetInterfaceObject(dat, "events");

        InterfaceToSim* sim = new InterfaceToSim(&SimulationManager, &EventsDataHub, &Config, GlobSet.RecNumTreads, false);
        QObject::connect(sim, SIGNAL(requestStopSimulation()), &SimulationManager, SLOT(StopSimulation()));
        GenScriptWindow.SetInterfaceObject(sim, "sim");

        InterfaceToReconstructor* rec = new InterfaceToReconstructor(&ReconstructionManager, &Config, &EventsDataHub, &TmpHub, GlobSet.RecNumTreads);
        QObject::connect(rec, SIGNAL(RequestStopReconstruction()), &ReconstructionManager, SLOT(requestStop()));
        GenScriptWindow.SetInterfaceObject(rec, "rec");

        AInterfaceToLRF* lrf = new AInterfaceToLRF(&Config, &EventsDataHub);
        GenScriptWindow.SetInterfaceObject(lrf, "lrf");
        ALrfScriptInterface* newLrf = new ALrfScriptInterface(&Detector, &EventsDataHub);
        GenScriptWindow.SetInterfaceObject(newLrf, "newLrf");

        AInterfaceToPMs* pmS = new AInterfaceToPMs(&Config);
        GenScriptWindow.SetInterfaceObject(pmS, "pms");

        AInterfaceToGraph* graph = new AInterfaceToGraph(&TmpHub);
        GenScriptWindow.SetInterfaceObject(graph, "graph");

        AInterfaceToHist* hist = new AInterfaceToHist(&TmpHub);
        GenScriptWindow.SetInterfaceObject(hist, "hist");

        AInterfaceToTree* tree = new AInterfaceToTree(&TmpHub);
        GenScriptWindow.SetInterfaceObject(tree, "tree");

        AInterfaceToMessageWindow* txt = new AInterfaceToMessageWindow(GenScriptWindow.ScriptManager, &GenScriptWindow);
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
        SM.SetInterfaceObject(0); //no replacement for the global object in "gloal script" mode
        AInterfaceToConfig* conf = new AInterfaceToConfig(&Config);
        SM.SetInterfaceObject(conf, "config");
        InterfaceToAddObjScript* geo = new InterfaceToAddObjScript(&Detector);
        SM.SetInterfaceObject(geo, "geo");
        AInterfaceToMinimizerScript* mini = new AInterfaceToMinimizerScript(&SM);
        SM.SetInterfaceObject(mini, "mini");  //mini should be before sim to handle abort correctly
        AInterfaceToData* dat = new AInterfaceToData(&Config, &EventsDataHub);
        SM.SetInterfaceObject(dat, "events");
#ifdef SIM
        InterfaceToSim* sim = new InterfaceToSim(&SimulationManager, &EventsDataHub, &Config, GlobSet.RecNumTreads, false);
        SM.SetInterfaceObject(sim, "sim");
#endif
        InterfaceToReconstructor* rec = new InterfaceToReconstructor(&ReconstructionManager, &Config, &EventsDataHub, &TmpHub, GlobSet.RecNumTreads);
        SM.SetInterfaceObject(rec, "rec");
        AInterfaceToLRF* lrf = new AInterfaceToLRF(&Config, &EventsDataHub);
        SM.SetInterfaceObject(lrf, "lrf");
        ALrfScriptInterface* newLrf = new ALrfScriptInterface(&Detector, &EventsDataHub);
        SM.SetInterfaceObject(newLrf, "newLrf");
        AInterfaceToPMs* pmS = new AInterfaceToPMs(&Config);
        SM.SetInterfaceObject(pmS, "pms");
        AInterfaceToGraph* graph = new AInterfaceToGraph(&TmpHub);
        SM.SetInterfaceObject(graph, "graph");
        AInterfaceToHist* hist = new AInterfaceToHist(&TmpHub);
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

