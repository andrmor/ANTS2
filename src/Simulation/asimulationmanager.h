#ifndef ASIMULATIONMANAGER_H
#define ASIMULATIONMANAGER_H

#include "generalsimsettings.h"
#include "atrackbuildoptions.h"

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

//class QJsonObject;
#include <QJsonObject>  // temporary

class ASimulationManager : public QObject
{
    Q_OBJECT
public:
    ASimulationManager(EventsDataClass & EventsDataHub, DetectorClass & Detector);
    ~ASimulationManager();

    bool isSimulationSuccess() const {return fSuccess;}
    bool isSimulationFinished() const {return fFinished;}
    bool isSimulationAborted() const;

    //last event info
    QVector<QBitArray> SiPMpixels;
    QVector<AEnergyDepositionCell *> EnergyVector;

    std::vector<TrackHolderClass *> Tracks;
    std::vector<ANodeRecord *> Nodes;

    std::vector<AEventTrackingRecord *> TrackingHistory;

    void StartSimulation(QJsonObject & json, int threads, bool fStartedFromGui);

    const QString & getErrorString() {return ErrorString;}
    void setErrorString(const QString & err) {ErrorString = err;}

    void clearTracks();
    void clearNodes();
    void clearEnergyVector();
    void clearTrackingHistory();

    void setMaxThreads(int maxThreads) {MaxThreads = maxThreads;}
    const QString loadNodesFromFile(const QString & fileName);

    void setG4Sim_OnlyGenerateFiles(bool flag) {bOnlyFileExport = flag;}
    bool isG4Sim_OnlyGenerateFiles() const {return bOnlyFileExport;}
    void generateG4antsConfigCommon(QJsonObject & json, int ThreadId);

    const DetectorClass & getDetector() {return Detector;}

    // Next three: Simulator workers use their own local copies constructed using configuration json
    ASourceParticleGenerator * ParticleSources = 0;         //used to update json on config changes and in GUI to configure
    AFileParticleGenerator   * FileParticleGenerator = 0;   //only for gui, simulation threads use their own
    AScriptParticleGenerator * ScriptParticleGenerator = 0; //only for gui, simulation threads use their own

    ATrackBuildOptions TrackBuildOptions;

    //for G4ants sims
    QSet<QString> SeenNonRegisteredParticles;
    double DepoByNotRegistered;
    double DepoByRegistered;

    GeneralSimSettings simSettings;
    QJsonObject jsSimSet;   // to be removed
    bool bPhotonSourceSim;  // if false -> particle source sim

private:
    EventsDataClass & EventsDataHub;
    DetectorClass   & Detector;

    ASimulatorRunner * Runner;
    QThread simRunnerThread;
    QTimer simTimerGuiUpdate;

    QString ErrorString; //temporary public!

    int  MaxThreads = -1;
    bool fStartedFromGui = false;
    bool fHardAborted = false;
    bool fFinished = true;
    bool fSuccess = false;

    bool bGuardTrackingHistory = false;

    // G4ants
    bool bOnlyFileExport = false; // single trigger flag

public slots:
    void onSimulationFinished(); //processing of simulation results!
    void StopSimulation();
    void onNewGeoManager(); // Nodes in history will be invalid after that!

private slots:
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
};

#endif // ASIMULATIONMANAGER_H
