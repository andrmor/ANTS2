//ANTS2
#include "detectorclass.h"
#include "eventsdataclass.h"
#include "areconstructionmanager.h"
#include "amaterialparticlecolection.h"
#include "tmpobjhubclass.h"
#include "aconfiguration.h"
#include "ajavascriptmanager.h"
#include "interfacetoglobscript.h"
#include "ainterfacetoaddobjscript.h"
#include "histgraphinterfaces.h"
#include "ainterfacetottree.h"
#include "scriptminimizer.h"
#include "globalsettingsclass.h"
#include "afiletools.h"
#include "aqtmessageredirector.h"
#include "particlesourcesclass.h"
#include "anetworkmodule.h"
#include "asandwich.h"
#include "amessageoutput.h"
#include "amessage.h"
#include "ainterfacetodeposcript.h"
#include "ainterfacetophotonscript.h"
#include "ainterfacetomultithread.h"
#include "ainterfacetowebsocket.h"
#include "awebserverinterface.h"

#ifdef ANTS_FLANN
  #include "ainterfacetoknnscript.h"
#endif
#ifdef ANTS_FANN
  #include "ainterfacetoannscript.h"
#endif

// SIM
#ifdef SIM
#include "simulationmanager.h"
#endif

// GUI
#ifdef GUI
#include "mainwindow.h"
#include "exampleswindow.h"
#include "ainterfacetomessagewindow.h"
#endif

//Qt
#include <QtWidgets/QApplication>
#include <QLocale>
#include <QDebug>
#include <QThread>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QLoggingCategory>
#include <QtMessageHandler>
#include <QCommandLineParser>
#include <QHostAddress>

//Root
#include "TApplication.h"
#include "TObject.h"
#include "TH1.h"
#include "RVersion.h"
#if ROOT_VERSION_CODE < ROOT_VERSION(6,11,1)
#include "TThread.h"
#endif

int main(int argc, char *argv[])
{
#ifdef GUI
    QApplication a(argc, argv);
#else
    QCoreApplication a(argc, argv);
#endif
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
    QObject::connect(&SimulationManager, &ASimulationManager::ProgressReport, &Network, &ANetworkModule::ProgressReport );
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
    else
#else // GUI
    if (argc > 1)
#endif // GUI
    {
        //direct script mode
        QCommandLineParser parser;
        parser.setApplicationDescription("ANTS2");
        parser.addHelpOption();
        parser.addPositionalArgument("scriptFile", QCoreApplication::translate("main", "File with the script to run"));
        parser.addPositionalArgument("outputFile", QCoreApplication::translate("main", "File with the console output"));
        parser.addPositionalArgument("ip", QCoreApplication::translate("main", "Web socket server IP"));
        parser.addPositionalArgument("port", QCoreApplication::translate("main", "Web socket server port"));
        parser.addPositionalArgument("ticket", QCoreApplication::translate("main", "Id for accessing ANTS2 server"));
        parser.addPositionalArgument("maxThreads", QCoreApplication::translate("main", "Maximum number of threads in sim and rec"));

        QCommandLineOption serverOption(QStringList() << "s" << "server",
                QCoreApplication::translate("main", "Run ANTS2 in server mode."));
        parser.addOption(serverOption);

        QCommandLineOption scriptOption(QStringList() << "f" << "file",
                QCoreApplication::translate("main", "Run script from <scriptFile>."),
                QCoreApplication::translate("main", "scriptFile"));
        parser.addOption(scriptOption);

        QCommandLineOption outputOption(QStringList() << "o" << "output",
                QCoreApplication::translate("main", "Redirect console to <outputFile>."),
                QCoreApplication::translate("main", "outputFile"));
        parser.addOption(outputOption);

        QCommandLineOption ipOption(QStringList() << "i" << "ip",
                QCoreApplication::translate("main", "Sets server IP."),
                QCoreApplication::translate("main", "ip"));
        parser.addOption(ipOption);

        QCommandLineOption portOption(QStringList() << "p" << "port",
                QCoreApplication::translate("main", "Sets server port."),
                QCoreApplication::translate("main", "port"));
        parser.addOption(portOption);

        QCommandLineOption ticketOption(QStringList() << "t" << "ticket",
                QCoreApplication::translate("main", "Sets server ticket."),
                QCoreApplication::translate("main", "ticket"));
        parser.addOption(ticketOption);

        QCommandLineOption maxThreadsOption(QStringList() << "m" << "maxThreads",
                QCoreApplication::translate("main", "Sets max sim and rec threads."),
                QCoreApplication::translate("main", "maxThreads"));
        parser.addOption(maxThreadsOption);

        parser.process(a);

        if ( parser.isSet(outputOption) )
        {
            QString LogFileName = parser.value(outputOption);
            qDebug() << "Redirecting console output to file: " << LogFileName;
            AQtMessageRedirector rd; //redirecting qDebug, qWarning etc to file
            if ( !rd.activate(LogFileName) )
            {
                qDebug() << "Failed to redirect output to file:"<<LogFileName;
                return 103;
            }
        }

        AJavaScriptManager SM(Detector.RandGen);
        Network.SetScriptManager(&SM);
        SM.SetInterfaceObject(0); //no replacement for the global object in "gloal script" mode
        AInterfaceToConfig* conf = new AInterfaceToConfig(&Config);
        SM.SetInterfaceObject(conf, "config");
        AInterfaceToAddObjScript* geo = new AInterfaceToAddObjScript(&Detector);
        SM.SetInterfaceObject(geo, "geo");
        AInterfaceToMinimizerJavaScript* mini = new AInterfaceToMinimizerJavaScript(&SM);
        SM.SetInterfaceObject(mini, "mini");  //mini should be before sim to handle abort correctly
        AInterfaceToData* dat = new AInterfaceToData(&Config, &EventsDataHub);
        SM.SetInterfaceObject(dat, "events");
#ifdef SIM
        InterfaceToSim* sim = new InterfaceToSim(&SimulationManager, &EventsDataHub, &Config, GlobSet.RecNumTreads, false);
        QObject::connect(sim, SIGNAL(requestStopSimulation()), &SimulationManager, SLOT(StopSimulation()));
        SM.SetInterfaceObject(sim, "sim");
#endif
        InterfaceToReconstructor* rec = new InterfaceToReconstructor(&ReconstructionManager, &Config, &EventsDataHub, &TmpHub, GlobSet.RecNumTreads);
        QObject::connect(rec, SIGNAL(RequestStopReconstruction()), &ReconstructionManager, SLOT(requestStop()));
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
        AInterfaceToTTree* tree = new AInterfaceToTTree(&TmpHub);
        SM.SetInterfaceObject(tree, "tree");
        AInterfaceToPhotonScript* photon = new AInterfaceToPhotonScript(&Config, &EventsDataHub);
        SM.SetInterfaceObject(photon, "photon");
        AInterfaceToDepoScript* depo = new AInterfaceToDepoScript(&Detector, &GlobSet, &EventsDataHub);
        SM.SetInterfaceObject(depo, "depo");
        AInterfaceToMultiThread* threads = new AInterfaceToMultiThread(&SM);
        SM.SetInterfaceObject(threads, "threads");
        AInterfaceToWebSocket* web = new AInterfaceToWebSocket(&EventsDataHub);
        SM.SetInterfaceObject(web, "web");
        AWebServerInterface* server = new AWebServerInterface(*Network.WebSocketServer, &EventsDataHub);
        SM.SetInterfaceObject(server, "server");
#ifdef ANTS_FLANN
        AInterfaceToKnnScript* knn = new AInterfaceToKnnScript(ReconstructionManager.KNNmodule);
        SM.SetInterfaceObject(knn, "knn");
#endif
#ifdef ANTS_FANN
        AInterfaceToANNScript* ann = new AInterfaceToANNScript();
        SM.SetInterfaceObject(ann, "ann");
#endif

        if ( parser.isSet(scriptOption) )
        {
            QString fileName = parser.value(scriptOption);
            qDebug() << "Running script from file:"<< fileName;
            if (!QFile(fileName).exists())
            {
                qDebug() << "File not found:"<<fileName;
                return 101;
            }

            QString script;
            bool ok = LoadTextFromFile(fileName, script);
            if (!ok)
            {
                qDebug() << "Failed to read script from file:"<<fileName;
                return 102;
            }

            QString path = QFileInfo(fileName).absolutePath();
            QDir::setCurrent(path);

            int errorLineNum = SM.FindSyntaxError(script); //qDebug is already inside
            if (errorLineNum > -1)
                return 0;

            QString result = SM.Evaluate(script);
            qDebug() << "Script evaluation result:"<<result;
            return 0;
        }
        else if ( parser.isSet(serverOption) )
        {
            if ( parser.isSet(maxThreadsOption) )
            {
                int max = parser.value(maxThreadsOption).toInt();
                SimulationManager.MaxThreads = max;
                ReconstructionManager.setMaxThread(max);
            }
            QHostAddress ip = QHostAddress::Null;
            if (parser.isSet(ipOption))
            {
                QString ips = parser.value(ipOption);
                ip = QHostAddress(ips);
            }
            if (ip.isNull())
            {
                qCritical("IP is not provided, exiting!");
                exit(12345);
            }
            quint16 Port = parser.value(portOption).toUShort();
            QString ticket = parser.value(ticketOption);
            qDebug() << "Starting server. IP ="<<ip.toString()<<"port ="<<Port<<"ticket ="<<ticket;
            if (!ticket.isEmpty()) Network.SetTicket(ticket);
            Network.SetExitOnDisconnect(true);
            Network.StartWebSocketServer(ip, Port);
            qDebug() << "To connect, use "<< Network.getWebSocketServerURL();
            a.exec();
            qDebug() << "Finished!"<<QTime::currentTime().toString();
        }
        else
        {
            qDebug() << "Unknown ANTS2 mode! Try ants2 -h";
            return 110;
        }
    }

    Network.StopRootHttpServer();
    Network.StopWebSocketServer();
}

