#ifndef GLOBALSETTINGSCLASS_H
#define GLOBALSETTINGSCLASS_H

#include <QObject>
#include <QJsonObject>

class ANetworkModule;

class GlobalSettingsClass
{
public:
  GlobalSettingsClass(ANetworkModule* NetModule);
  ~GlobalSettingsClass();

  void SaveANTSconfiguration();
  void LoadANTSconfiguration();
  void writeToJson(QJsonObject& json);
  void readFromJson(QJsonObject& json);

  QString AntsBaseDir; // QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/ants2";
  QString ConfigDir;   // AntsbaseDir + "/Config"
  QString QuicksaveDir;// AntsbaseDir + "/Quicksave"
  QString ExamplesDir; // AntsbaseDir + "/Examples"
  QString TmpDir;      // AntsbaseDir + "/Tmp"

  QString LastOpenDir; // last open directory using save and load dialog
  QString LibMaterials;
  QString LibPMtypes;
  QString LibParticleSources;
  QString LibScripts;

  int FontSize;
  int TextLogPrecision;
  int BinsX, BinsY, BinsZ;
  int FunctionPointsX, FunctionPointsY;
  int NumSegments;     // number of segments in drawing, e.g., round objects
  int MaxNumberOfTracks;

  bool ShowExamplesOnStart; //pop-up examples window
  bool SaveLoadWindows; //save windows on exit, load them on ANTS2 start
  bool AlwaysSaveOnExit; //save detector to QuickSave on exit, do not ask
  bool NeverSaveOnExit; //do not save detector on exit, do not ask

  int NumThreads;
  int RecNumTreads;

  bool RecTreeSave_IncludePMsignals;
  bool RecTreeSave_IncludeRho;
  bool RecTreeSave_IncludeTrue;  

  bool PerformAutomaticGeometryCheck;

  //GUI flags
  bool fOpenImageExternalEditor;

  QString RootStyleScript;

  //GlobScript
  QString GlobScript;
  QJsonObject ScriptWindowJson;

  //Network
  int DefaultWebSocketPort;
  bool fRunWebSocketServerOnStart;
  int DefaultRootServerPort;
  bool fRunRootServerOnStart;
  QString ExternalJSROOT;

  ANetworkModule* NetModule;
};

#endif // GLOBALSETTINGSCLASS_H
