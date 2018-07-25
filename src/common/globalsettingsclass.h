#ifndef GLOBALSETTINGSCLASS_H
#define GLOBALSETTINGSCLASS_H

#include <QObject>
#include <QJsonObject>

class ANetworkModule;

class GlobalSettingsClass
{
public:
  GlobalSettingsClass(ANetworkModule* NetModule);

  void SaveANTSconfiguration();
  void LoadANTSconfiguration();
  void writeToJson(QJsonObject& json);
  void readFromJson(QJsonObject& json);

  QString AntsBaseDir; // QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/ants2";
  QString ConfigDir;   // AntsbaseDir + "/Config"
  QString QuicksaveDir;// AntsbaseDir + "/Quicksave"
  QString ExamplesDir; // AntsbaseDir + "/Examples"
  QString ResourcesDir; // AntsbaseDir + "/Examples"
  QString TmpDir;      // AntsbaseDir + "/Tmp"

  QString LastOpenDir; // last open directory using save and load dialog
  QString LibMaterials;
  QString LibPMtypes;
  QString LibParticleSources;
  QString LibScripts;

  int FontSize = 8;
  int TextLogPrecision = 4;
  int BinsX = 100;
  int BinsY = 100;
  int BinsZ = 100;
  int FunctionPointsX = 30;
  int FunctionPointsY = 30;
  int NumSegments = 20;     // number of segments in drawing, e.g., round objects
  int MaxNumberOfTracks = 1000;

  bool ShowExamplesOnStart = true; //pop-up examples window
  bool SaveLoadWindows = true; //save windows on exit, load them on ANTS2 start
  //bool AlwaysSaveOnExit; //save detector to QuickSave on exit, do not ask
  //bool NeverSaveOnExit; //do not save detector on exit, do not ask

  int NumThreads = -1; //if not loaded -> -1 means use recommended settings
  int RecNumTreads = 1;

  bool RecTreeSave_IncludePMsignals = true;
  bool RecTreeSave_IncludeRho = true;
  bool RecTreeSave_IncludeTrue = true;

  bool SimTextSave_IncludeNumPhotons = true;
  bool SimTextSave_IncludePositions = true;

  bool PerformAutomaticGeometryCheck = true;

  bool fOpenImageExternalEditor = true;

  QJsonObject MaterialsAndParticlesSettings;

  QString RootStyleScript;

  //GlobScript
  QString GlobScript;
  QJsonObject ScriptWindowJson;
  QJsonObject PythonScriptWindowJson;
  int DefaultFontSize_ScriptWindow = 12;
  QString DefaultFontFamily_ScriptWindow; //empty => Qt standard settings will be used
  bool DefaultFontWeight_ScriptWindow = false;
  bool DefaultFontItalic_ScriptWindow = false;

  //Network
  int DefaultWebSocketPort = 1234;
  QString DefaultWebSocketIP = "127.0.0.1";
  int DefaultRootServerPort = 8080;
  bool fRunRootServerOnStart = false;
  QString ExternalJSROOT;

  //RemoteServers
  QJsonObject RemoteServers;

  ANetworkModule* NetModule;
};

#endif // GLOBALSETTINGSCLASS_H
