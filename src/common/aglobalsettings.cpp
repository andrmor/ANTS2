#include "aglobalsettings.h"
#include "aisotopeabundancehandler.h"
#include "ajsontools.h"
#include "anetworkmodule.h"
#include "ajavascriptmanager.h"
#include "amessage.h"
#include "agstyle_si.h"

#ifdef GUI
#include "globalsettingswindowclass.h"
#endif

#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QtWidgets/QApplication>
#include <QThread>
#include "TStyle.h"

AGlobalSettings& AGlobalSettings::getInstance()
{
    static AGlobalSettings instance;
    return instance;
}

AGlobalSettings::AGlobalSettings()
{
    RecNumTreads = QThread::idealThreadCount() - 1;
    if (RecNumTreads < 1) RecNumTreads = 1;

// Default Geant4 settings for docker installation
#ifdef ANTS_DOCKER
    G4antsExec = "/G4ants/G4ants";
    G4ExchangeFolder = "/ants_config/ants2/Tmp";
#endif

    //default font size
#ifdef Q_OS_LINUX // fix font size on Linux
    FontSize = 8;
#else
    FontSize = 8;
#endif

    //setting up directories
#ifdef WINDOWSBIN
    QDir dir = QDir::current();
    dir.cdUp();
    AntsBaseDir = dir.absolutePath();
    QDir::setCurrent(AntsBaseDir);
#else
    //base is suggested by Qt + "/ants2"
    AntsBaseDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/ants2";
#endif

    if (!QDir(AntsBaseDir).exists()) QDir().mkdir(AntsBaseDir);
    QuicksaveDir = AntsBaseDir + "/Quicksave"; //dir for quicksave
    if (!QDir(QuicksaveDir).exists()) QDir().mkdir(QuicksaveDir);
    ExamplesDir = QDir::current().absolutePath() + "/EXAMPLES"; //dir where examples will be copied
    ResourcesDir = QDir::current().absolutePath() + "/DATA";//dir where data will be copied

    IsotopeAbundanceHandler = new AIsotopeAbundanceHandler( ResourcesDir + "/Neutrons/IsotopeNaturalAbundances.txt" );

#ifdef Q_OS_WIN32
    if (!QDir(ExamplesDir).exists())  //direct call of ants2.exe
    {
        QDir dir = QDir::current();
        dir.cdUp();
        QString candidate = dir.absolutePath() + "/EXAMPLES";
        if (QDir(candidate).exists()) ExamplesDir = candidate;

        candidate = dir.absolutePath() + "/DATA";
        if (QDir(candidate).exists()) ResourcesDir = candidate;
    }
#endif

    TmpDir = AntsBaseDir + "/Tmp"; //dir for tmp saves
    if (!QDir(TmpDir).exists()) QDir().mkdir(TmpDir);
    QDir::setCurrent(TmpDir);
    LastOpenDir = TmpDir; //setting current directory for load/save dialogs - LoadANTSconfiguration can override
    ConfigDir = AntsBaseDir + "/Config"; //dir for ANTS2 settings

    //natutal abundances data default filename
    MaterialsAndParticlesSettings["NaturalAbundanciesFileName"] = ResourcesDir+"/Neutrons/IsotopeNaturalAbundances.txt";
    MaterialsAndParticlesSettings["CrossSectionsDir"] = ResourcesDir+"/Neutrons/CrossSections";
    MaterialsAndParticlesSettings["EnableAutoLoad"] = true;

    //if exists,load file fith ANTS2 settings, otherwise create config dir
    if (!QDir(ConfigDir).exists())
    {
        qDebug() << "Config dir not found, skipping config load, creating new config dir" ;
        QDir().mkdir(ConfigDir); //dir not found, skipping load config
    }
    else loadANTSconfiguration();

    if (LibLogs.isEmpty()) LibLogs = AntsBaseDir + "/Logs";
    if (!QDir(LibLogs).exists()) QDir().mkdir(LibLogs);

#ifdef GUI
    if (!RootStyleScript.isEmpty())
    {
        //running root TStyle script
        AJavaScriptManager* SM = new AJavaScriptManager(0);
        AGStyle_SI* GStyleInterface  = new  AGStyle_SI(); //deleted by the SM
        SM->RegisterInterfaceAsGlobal(GStyleInterface);
        SM->Evaluate(RootStyleScript);
        SM->deleteLater();
    }
    gStyle->SetOptTitle(0);  // disables drawing of the title of ROOT histograms / graphs
#endif
}

AGlobalSettings::~AGlobalSettings()
{
    delete IsotopeAbundanceHandler; IsotopeAbundanceHandler = 0;
}

void AGlobalSettings::writeToJson(QJsonObject &json) const
{
    QJsonObject js;
    js["SaveLoadWindows"] = SaveLoadWindows;
    js["ShowExamplesOnStart"] = ShowExamplesOnStart;
    js["LastDir"] = LastOpenDir;
    js["PMtypeLib"] = LibPMtypes;
    js["MaterialLib"] = LibMaterials;
    js["ParticleSourcesLib"] = LibParticleSources;
    js["ScriptsLib"] = LibScripts;
    js["LogsLib"] = LibLogs;

    js["FontSize"] = FontSize;
    js["RootStyleScript"] = RootStyleScript;
    js["OpenImageExternalEditor"] = fOpenImageExternalEditor;
    js["TextLogPrecision"] = TextLogPrecision;
    js["BinsX"] = BinsX;
    js["BinsY"] = BinsY;
    js["BinsZ"] = BinsZ;
    js["FunctionPointsX"] = FunctionPointsX;
    js["FunctionPointsY"] = FunctionPointsY;
    js["NumSegments"] = NumSegments;
    js["PerformAutomaticGeometryCheck"] = PerformAutomaticGeometryCheck;
    js["NumThreads"] = NumThreads;

    js["MaterialsAndParticlesSettings"] = MaterialsAndParticlesSettings;

    js["RecTreeSave_IncludePMsignals"] = RecTreeSave_IncludePMsignals;
    js["RecTreeSave_IncludeRho"] = RecTreeSave_IncludeRho;
    js["RecTreeSave_IncludeTrue"] = RecTreeSave_IncludeTrue;

    js["SimTextSave_IncludeNumPhotons"] = SimTextSave_IncludeNumPhotons;
    js["SimTextSave_IncludePositions"] = SimTextSave_IncludePositions;

    js["GlobScript"] = GlobScript;
    js["ScriptWindowJson"] = ScriptWindowJson;
    js["PythonScriptWindowJson"] = PythonScriptWindowJson;
    js["DefaultFontSize_ScriptWindow"] = DefaultFontSize_ScriptWindow;
    js["DefaultFontFamily_ScriptWindow"] = DefaultFontFamily_ScriptWindow;
    js["DefaultFontWeight_ScriptWindow"] = DefaultFontWeight_ScriptWindow;
    js["DefaultFontItalic_ScriptWindow"] = DefaultFontItalic_ScriptWindow;

    js["DefaultWebSocketPort"] = DefaultWebSocketPort;
    js["DefaultWebSocketIP"] = DefaultWebSocketIP;
    js["RootServerPort"] = RootServerPort;
    js["RunRootServerOnStart"] = fRunRootServerOnStart;
    js["ExternalJSROOT"] = ExternalJSROOT;

    js["RemoteServers"] = RemoteServers;

    js["G4ants"] = G4antsExec;
    js["G4ExchangeFolder"] = G4ExchangeFolder;

    json["ANTS2config"] = js;
}

void AGlobalSettings::readFromJson(const QJsonObject &json)
{
    QJsonObject js = json["ANTS2config"].toObject();

    parseJson(js, "SaveLoadWindows", SaveLoadWindows);
    parseJson(js, "ShowExamplesOnStart", ShowExamplesOnStart);
    parseJson(js, "LastDir", LastOpenDir);
    parseJson(js, "PMtypeLib", LibPMtypes);
    parseJson(js, "MaterialLib", LibMaterials);
    parseJson(js, "ParticleSourcesLib", LibParticleSources);
    parseJson(js, "ScriptsLib", LibScripts);
    parseJson(js, "LogsLib", LibLogs);

    parseJson(js, "FontSize", FontSize);
    parseJson(js, "RootStyleScript", RootStyleScript);
    parseJson(js, "OpenImageExternalEditor", fOpenImageExternalEditor);
    parseJson(js, "TextLogPrecision", TextLogPrecision);
    parseJson(js, "BinsX", BinsX);
    parseJson(js, "BinsY", BinsY);
    parseJson(js, "BinsZ", BinsZ);
    parseJson(js, "FunctionPointsX", FunctionPointsX);
    parseJson(js, "FunctionPointsY", FunctionPointsY);
    parseJson(js, "NumSegments", NumSegments);
    parseJson(js, "NumThreads", NumThreads);

    parseJson(js, "RecTreeSave_IncludePMsignals", RecTreeSave_IncludePMsignals);
    parseJson(js, "RecTreeSave_IncludeRho", RecTreeSave_IncludeRho);
    parseJson(js, "RecTreeSave_IncludeTrue", RecTreeSave_IncludeTrue);

    parseJson(js, "SimTextSave_IncludeNumPhotons", SimTextSave_IncludeNumPhotons);
    parseJson(js, "SimTextSave_IncludePositions", SimTextSave_IncludePositions);

    parseJson(js, "PerformAutomaticGeometryCheck", PerformAutomaticGeometryCheck);

    parseJson(js, "GlobScript", GlobScript);
    if (js.contains("ScriptWindowJson"))
        ScriptWindowJson = js["ScriptWindowJson"].toObject();
    if (js.contains("PythonScriptWindowJson"))
        PythonScriptWindowJson = js["PythonScriptWindowJson"].toObject();

    parseJson(js, "MaterialsAndParticlesSettings", MaterialsAndParticlesSettings);

    parseJson(js, "DefaultFontSize_ScriptWindow", DefaultFontSize_ScriptWindow);
    parseJson(js, "DefaultFontFamily_ScriptWindow", DefaultFontFamily_ScriptWindow);
    parseJson(js, "DefaultFontWeight_ScriptWindow", DefaultFontWeight_ScriptWindow);
    parseJson(js, "DefaultFontItalic_ScriptWindow", DefaultFontItalic_ScriptWindow);

    //network
    parseJson(js, "DefaultWebSocketPort", DefaultWebSocketPort);
    parseJson(js, "DefaultWebSocketIP", DefaultWebSocketIP);
    parseJson(js, "DefaultRootServerPort", RootServerPort); //compatibility
    parseJson(js, "RootServerPort", RootServerPort);
    parseJson(js, "RunRootServerOnStart", fRunRootServerOnStart);

    parseJson(js, "RemoteServers", RemoteServers);

    parseJson(js, "G4ants", G4antsExec);
    parseJson(js, "G4ExchangeFolder", G4ExchangeFolder);
    if (G4ExchangeFolder.isEmpty())
        G4ExchangeFolder = TmpDir;

    QString tmp;
    parseJson(js, "ExternalJSROOT", tmp);
    if (!tmp.isEmpty()) ExternalJSROOT = tmp;
}

void AGlobalSettings::saveANTSconfiguration() const
{
    QString fileName = ConfigDir + "/config.ini";
    QJsonObject json;
    writeToJson(json);
    SaveJsonToFile(json, fileName);
}

void AGlobalSettings::loadANTSconfiguration()
{
    QString fileName = ConfigDir + "/config.ini";
    QJsonObject json;
    LoadJsonFromFile(json, fileName);
    if (!json.isEmpty()) readFromJson(json);
}
