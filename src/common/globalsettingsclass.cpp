#include "globalsettingsclass.h"
#include "ajsontools.h"
#include "anetworkmodule.h"
#include "ajavascriptmanager.h"
#include "amessage.h"

#ifdef GUI
#include "globalsettingswindowclass.h"
#endif

#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QApplication>
#include <QThread>

GlobalSettingsClass::GlobalSettingsClass(ANetworkModule *NetModule) : NetModule(NetModule)
{
  SaveLoadWindows = true;
  ShowExamplesOnStart = true;
  PerformAutomaticGeometryCheck = true;

  fOpenImageExternalEditor = true;
  TextLogPrecision = 4;
  BinsX = BinsY = BinsZ = 100;
  FunctionPointsX = FunctionPointsY = 30;
  NumSegments = 20;
  MaxNumberOfTracks = 1000;

  DefaultFontSize_ScriptWindow = 12;
  DefaultFontFamily_ScriptWindow = ""; //undefined - Qt standard settings will be used
  DefaultFontWeight_ScriptWindow = false;
  DefaultFontItalic_ScriptWindow = false;

  NumThreads = -1; //if not loaded - set it using recommended settings
  RecNumTreads = QThread::idealThreadCount()-1;
  if (RecNumTreads<1) RecNumTreads = 1;

  RecTreeSave_IncludePMsignals = true;
  RecTreeSave_IncludeRho = true;
  RecTreeSave_IncludeTrue = true;

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
//  qDebug() << "-base-"<<AntsBaseDir;

    //dir for quicksave
  QuicksaveDir = AntsBaseDir + "/Quicksave";
  if (!QDir(QuicksaveDir).exists()) QDir().mkdir(QuicksaveDir);
//  qDebug() << "-qsave-"<<QuicksaveDir;

    //dir where examples will be copied
  ExamplesDir = QDir::current().absolutePath() + "/EXAMPLES";

#ifdef Q_OS_WIN32
  if (!QDir(ExamplesDir).exists())  //direct call of ants2.exe
  {
      QDir dir = QDir::current();
      dir.cdUp();
      QString candidate = dir.absolutePath() + "/EXAMPLES";
      if (QDir(candidate).exists())
          ExamplesDir = candidate;
  }
#endif
//   qDebug() << "-examples-"<<ExamplesDir;

    //dir for tmp saves
  TmpDir = AntsBaseDir + "/Tmp";
  if (!QDir(TmpDir).exists()) QDir().mkdir(TmpDir);
//  qDebug() << "-tmp-"<<TmpDir;
  QDir::setCurrent(TmpDir);
//  qDebug() << "-current-"<<QDir::current().dirName()<< "should be the same as -tmp- on start";
  LastOpenDir = TmpDir; //setting current directory for load/save dialogs - LoadANTSconfiguration can override

    //dir for ANTS2 settings
  ConfigDir = AntsBaseDir + "/Config";
//  qDebug() << "-conf-"<<ConfigDir;

  //natutal abundances data default filename
  MaterialsAndParticlesSettings["NaturalAbundanciesFile"] = ExamplesDir+"/"+"IsotopeNaturalAbundances.txt";

  //if exists,load file fith ANTS2 settings, otherwise create config dir
  if (!QDir(ConfigDir).exists())
    {
      qDebug() << "Config dir not found, skipping config load, creating new config dir" ;
      QDir().mkdir(ConfigDir); //dir not found, skipping load config
    }
  else
     LoadANTSconfiguration();

  //running root TStyle script  
  if (!RootStyleScript.isEmpty())
  {
      AJavaScriptManager* SM = new AJavaScriptManager(0);
#ifdef GUI
      InterfaceToGStyleScript* GStyleInterface  = new  InterfaceToGStyleScript(); //deleted by the SM
      SM->SetInterfaceObject(GStyleInterface);
#endif
      SM->Evaluate(RootStyleScript);
      SM->deleteLater();
  }
}

void GlobalSettingsClass::writeToJson(QJsonObject &json)
{
  QJsonObject js;
  js["SaveLoadWindows"] = SaveLoadWindows;
  js["ShowExamplesOnStart"] = ShowExamplesOnStart;
  //js["AlwaysSaveOnExit"] = AlwaysSaveOnExit;
  //js["NeverSaveOnExit"] = NeverSaveOnExit;
  js["LastDir"] = LastOpenDir;
  js["PMtypeLib"] = LibPMtypes;
  js["MaterialLib"] = LibMaterials;
  js["ParticleSourcesLib"] = LibParticleSources;
  js["ScriptsLib"] = LibScripts;
  //QFont font = this->font();
  //int size = font.pointSize();
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
  js["MaxNumberOfTracks"] = MaxNumberOfTracks;
  js["PerformAutomaticGeometryCheck"] = PerformAutomaticGeometryCheck;
  js["NumThreads"] = NumThreads;

  js["MaterialsAndParticlesSettings"] = MaterialsAndParticlesSettings;

  js["RecTreeSave_IncludePMsignals"] = RecTreeSave_IncludePMsignals;
  js["RecTreeSave_IncludeRho"] = RecTreeSave_IncludeRho;
  js["RecTreeSave_IncludeTrue"] = RecTreeSave_IncludeTrue;

  js["GlobScript"] = GlobScript;
  js["ScriptWindowJson"] = ScriptWindowJson;
  js["DefaultFontSize_ScriptWindow"] = DefaultFontSize_ScriptWindow;
  js["DefaultFontFamily_ScriptWindow"] = DefaultFontFamily_ScriptWindow;
  js["DefaultFontWeight_ScriptWindow"] = DefaultFontWeight_ScriptWindow;
  js["DefaultFontItalic_ScriptWindow"] = DefaultFontItalic_ScriptWindow;
  QJsonArray mspAr;
  for (int w : MainSplitterSizes_ScriptWindow) mspAr << w;
  js["MainSplitterSizes_ScriptWindow"] = mspAr;

  js["DefaultWebSocketPort"] = DefaultWebSocketPort;
  js["RunWebSocketServerOnStart"] = fRunWebSocketServerOnStart;
  js["DefaultRootServerPort"] = DefaultRootServerPort;
  js["RunRootServerOnStart"] = fRunRootServerOnStart;
  js["ExternalJSROOT"] = ExternalJSROOT;

  json["ANTS2config"] = js;
}

void GlobalSettingsClass::readFromJson(QJsonObject &json)
{
    QJsonObject js = json["ANTS2config"].toObject();

    parseJson(js, "SaveLoadWindows", SaveLoadWindows);
    //parseJson(js, "AlwaysSaveOnExit", AlwaysSaveOnExit);
    //parseJson(js, "NeverSaveOnExit", NeverSaveOnExit);
    parseJson(js, "ShowExamplesOnStart", ShowExamplesOnStart);
    parseJson(js, "LastDir", LastOpenDir);
    parseJson(js, "PMtypeLib", LibPMtypes);
    parseJson(js, "MaterialLib", LibMaterials);
    parseJson(js, "ParticleSourcesLib", LibParticleSources);
    parseJson(js, "ScriptsLib", LibScripts);
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
    parseJson(js, "MaxNumberOfTracks", MaxNumberOfTracks);
    parseJson(js, "NumThreads", NumThreads);

    parseJson(js, "RecTreeSave_IncludePMsignals", RecTreeSave_IncludePMsignals);
    parseJson(js, "RecTreeSave_IncludeRho", RecTreeSave_IncludeRho);
    parseJson(js, "RecTreeSave_IncludeTrue", RecTreeSave_IncludeTrue);

    parseJson(js, "PerformAutomaticGeometryCheck", PerformAutomaticGeometryCheck);

    parseJson(js, "GlobScript", GlobScript);
    if (js.contains("ScriptWindowJson"))
        ScriptWindowJson = js["ScriptWindowJson"].toObject();

    parseJson(js, "MaterialsAndParticlesSettings", MaterialsAndParticlesSettings);

    parseJson(js, "DefaultFontSize_ScriptWindow", DefaultFontSize_ScriptWindow);
    parseJson(js, "DefaultFontFamily_ScriptWindow", DefaultFontFamily_ScriptWindow);
    parseJson(js, "DefaultFontWeight_ScriptWindow", DefaultFontWeight_ScriptWindow);
    parseJson(js, "DefaultFontItalic_ScriptWindow", DefaultFontItalic_ScriptWindow);
    QJsonArray mspAr;
    parseJson(js, "MainSplitterSizes_ScriptWindow", mspAr);
    for (int imsa=0; imsa<mspAr.size(); imsa++)
        MainSplitterSizes_ScriptWindow << mspAr[imsa].toInt(50);

    //network
    DefaultWebSocketPort = 1234;
    fRunWebSocketServerOnStart = false;
    DefaultRootServerPort = 8080;
    fRunRootServerOnStart = false;
    ExternalJSROOT = "https://root.cern/js/latest/";

    parseJson(js, "DefaultWebSocketPort", DefaultWebSocketPort);
    parseJson(js, "RunWebSocketServerOnStart", fRunWebSocketServerOnStart);
    parseJson(js, "DefaultRootServerPort", DefaultRootServerPort);
    parseJson(js, "RunRootServerOnStart", fRunRootServerOnStart);
    parseJson(js, "ExternalJSROOT", ExternalJSROOT);
}

void GlobalSettingsClass::SaveANTSconfiguration()
{
  QString fileName = ConfigDir + "/config.ini";
  QJsonObject json;
  writeToJson(json);
  SaveJsonToFile(json, fileName);
}

void GlobalSettingsClass::LoadANTSconfiguration()
{
  QString fileName = ConfigDir + "/config.ini";
  QJsonObject json;
  LoadJsonFromFile(json, fileName);
  if (!json.isEmpty()) readFromJson(json);
}
