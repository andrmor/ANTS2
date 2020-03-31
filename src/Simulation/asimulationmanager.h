#ifndef ASIMULATIONMANAGER_H
#define ASIMULATIONMANAGER_H

#include "generalsimsettings.h"
#include "atrackbuildoptions.h"
#include "alogsandstatisticsoptions.h"

#include <vector>

#include <QObject>
#include <QString>
#include <QTimer>
#include <QSet>
#include <QThread>

class EventsDataClass;
class DetectorClass;
class ASimulatorRunner;
class AEnergyDepositionCell;
class TrackHolderClass;
class ANodeRecord;
class ASourceParticleGenerator;
class AFileParticleGenerator;
class AScriptParticleGenerator;
class AEventTrackingRecord;
class ASimulator;

//class QJsonObject;
#include <QJsonObject>  // temporary

class ASimulationManager : public QObject
{
    Q_OBJECT
public:
    ASimulationManager(EventsDataClass & EventsDataHub, DetectorClass & Detector);
    ~ASimulationManager();

    void StartSimulation(QJsonObject & json, int threads, bool bDoGuiUpdate);
    // when simulation is finished, private slot ASimulationManager::onSimulationFinished() is triggered

    bool isSimulationSuccess() const {return fSuccess;}
    bool isSimulationFinished() const {return fFinished;}
    bool isSimulationAborted() const;

    const QString & getErrorString() {return ErrorString;}
    void setErrorString(const QString & err) {ErrorString = err;}

    void clearTracks();
    void clearNodes();
    void clearEnergyVector();
    void clearTrackingHistory();

    void setMaxThreads(int maxThreads) {MaxThreads = maxThreads;}
    const QString loadNodesFromFile(const QString & fileName);

    //void setG4Sim_OnlyGenerateFiles(bool flag) {bOnlyFileExport = flag;}
    //bool isG4Sim_OnlyGenerateFiles() const {return bOnlyFileExport;}
    void generateG4antsConfigCommon(QJsonObject & json, ASimulator * worker);  // !!! G4ants files common

    const DetectorClass & getDetector() {return Detector;}

public:
    std::vector<ANodeRecord *> Nodes;

    //history
    std::vector<TrackHolderClass *> Tracks;
    std::vector<AEventTrackingRecord *> TrackingHistory;

    //last event info
    QVector<QBitArray> SiPMpixels;
    QVector<AEnergyDepositionCell *> EnergyVector;

    // Next three: Simulator workers use their own local copies constructed using configuration json
    ASourceParticleGenerator * ParticleSources = nullptr;         //used to update json on config changes and in GUI to configure
    AFileParticleGenerator   * FileParticleGenerator = nullptr;   //only for gui, simulation threads use their own
    AScriptParticleGenerator * ScriptParticleGenerator = nullptr; //only for gui, simulation threads use their own

    ATrackBuildOptions TrackBuildOptions;
    ALogsAndStatisticsOptions LogsStatOptions;

    //for G4ants sims
    QSet<QString> SeenNonRegisteredParticles;
    double DepoByNotRegistered;
    double DepoByRegistered;

    GeneralSimSettings simSettings;
    QJsonObject jsSimSet;   // to be removed
    bool bPhotonSourceSim;  // if false -> particle source sim

    int NumberOfWorkers = 0;

private:
    EventsDataClass & EventsDataHub;
    DetectorClass   & Detector;

    ASimulatorRunner * Runner = nullptr;

    QThread simRunnerThread;
    QTimer simTimerGuiUpdate;

    QString ErrorString;

    int  MaxThreads = -1;
    bool bDoGuiUpdate = false;
    bool fHardAborted = false;
    bool fFinished = true;
    bool fSuccess = false;

    bool bGuardTrackingHistory = false;

    // G4ants
    //bool bOnlyFileExport = false; // single trigger flag

public slots:
    void StopSimulation();
    void onNewGeoManager(); // Nodes in history will be invalid after that!

private slots:
    void onSimulationFinished(); //processing of simulation results! ++++++++++++++++++++++++
    void onSimFailedToStart();    
    void updateGui();

signals:
    void updateReady(int Progress, double msPerEvent);
    void RequestStopSimulation();
    void SimulationFinished();
    void ProgressReport(int percents);

private:
    bool setup(const QJsonObject & json, int threads);
    void clearG4data();
    void copyDataFromWorkers();
    void removeOldFile(const QString &fileName, const QString &txt);
    const QString makeLogDir() const;
    void saveParticleLog(const QString & dir) const;
    void saveG4ParticleLog(const QString & dir) const;
    void saveA2ParticleLog(const QString & dir) const;
    void saveDepositionLog(const QString & dir) const;
    void saveG4depositionLog(const QString & dir) const;
    void saveA2depositionLog(const QString & dir) const;
    void saveExitLog();
};

#endif // ASIMULATIONMANAGER_H
