#ifndef AGLOBALSETTINGS_H
#define AGLOBALSETTINGS_H

#include <QString>
#include <QJsonObject>

class AIsotopeAbundanceHandler;
class ANetworkModule;
class AMaterialParticleCollection;

class AGlobalSettings final
{
public:
    static AGlobalSettings& getInstance();

private:
    AGlobalSettings();
    ~AGlobalSettings();

    AGlobalSettings(const AGlobalSettings&) = delete;           // Copy ctor
    AGlobalSettings(AGlobalSettings&&) = delete;                // Move ctor
    AGlobalSettings& operator=(const AGlobalSettings&) = delete;// Copy assign
    AGlobalSettings& operator=(AGlobalSettings&&) = delete;     // Move assign

public:
    void saveANTSconfiguration() const;
    void loadANTSconfiguration();

    void writeToJson(QJsonObject& json) const;
    void readFromJson(const QJsonObject& json);

    AIsotopeAbundanceHandler & getIsotopeAbundanceHandler() const {return *IsotopeAbundanceHandler;}

    void setNetworkModule(ANetworkModule* NetworkModule) {NetModule = NetworkModule;}
    ANetworkModule* getNetworkModule() const {return NetModule;}

    //the next 2 lines are here until AConfiguration becomes singleton
    void setMpCollection(AMaterialParticleCollection * MPCollection) {MpCollection = MPCollection;}
    AMaterialParticleCollection * getMpCollection() {return MpCollection;}

    QString AntsBaseDir;  // QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/ants2";
    QString ConfigDir;    // AntsbaseDir + "/Config"
    QString QuicksaveDir; // AntsbaseDir + "/Quicksave"
    QString ExamplesDir;  // AntsbaseDir + "/Examples"
    QString ResourcesDir; // AntsbaseDir + "/DATA"
    QString TmpDir;       // AntsbaseDir + "/Tmp"

    QString LastOpenDir; // last open directory using save and load dialog
    QString LibMaterials;
    QString LibPMtypes;
    QString LibParticleSources;
    QString LibScripts;
    QString LibLogs;

    int FontSize = 8;
    int TextLogPrecision = 4;
    int BinsX = 100;
    int BinsY = 100;
    int BinsZ = 100;
    int FunctionPointsX = 30;
    int FunctionPointsY = 30;
    int NumSegments = 20;     // number of segments in drawing, e.g., round objects

    bool ShowExamplesOnStart = true; //pop-up examples window
    bool SaveLoadWindows = true; //save windows on exit, load them on ANTS2 start

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
    int RootServerPort = 8080;
    QString ExternalJSROOT = "https://root.cern/js/latest/";
    bool fRunRootServerOnStart = false;

    //RemoteServers
    QJsonObject RemoteServers;

    //Geant4 interface
    QString G4antsExec;
    QString G4ExchangeFolder;

private:
    AIsotopeAbundanceHandler    * IsotopeAbundanceHandler = nullptr; //owns
    ANetworkModule              * NetModule               = nullptr; //external
    AMaterialParticleCollection * MpCollection            = nullptr; //external
};

#endif // AGLOBALSETTINGS_H
