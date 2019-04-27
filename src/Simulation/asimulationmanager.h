#ifndef ASIMULATIONMANAGER_H
#define ASIMULATIONMANAGER_H

#include "atrackbuildoptions.h"

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

class QJsonObject;

class ASimulationManager : public QObject
{
    Q_OBJECT
public:
    ASimulationManager(EventsDataClass & EventsDataHub, DetectorClass & Detector);
    ~ASimulationManager();

    bool isSimulationSuccess() const {return fSuccess;}
    bool isSimulationFinished() const {return fFinished;}
    bool isSimulationAborted() const;

    int LastSimType; // -1 - undefined, 0 - PointSources, 1 - ParticleSources

    //last event info
    QVector< QBitArray > SiPMpixels;
    QVector<AEnergyDepositionCell *> EnergyVector;

    std::vector<TrackHolderClass *> Tracks;
    std::vector<ANodeRecord *> Nodes;

    void StartSimulation(QJsonObject & json, int threads, bool fStartedFromGui);

    const QString & getErrorString() {return ErrorString;}

    void clearTracks();
    void clearNodes();
    void clearEnergyVector();

    void setMaxThreads(int maxThreads) {MaxThreads = maxThreads;}
    const QString loadNodesFromFile(const QString & fileName);

    // Next three: Simulator workers use their own local copies constructed using configuration json
    ASourceParticleGenerator * ParticleSources = 0;         //used to update json on config changes and in GUI to configure
    AFileParticleGenerator   * FileParticleGenerator = 0;   //only for gui, simulation threads use their own
    AScriptParticleGenerator * ScriptParticleGenerator = 0; //only for gui, simulation threads use their own

    ATrackBuildOptions TrackBuildOptions;

    //for G4ants sims
    QSet<QString> SeenNonRegisteredParticles;
    double DepoByNotRegistered;
    double DepoByRegistered;

    QString ErrorString; //temporary public!

private:
    EventsDataClass & EventsDataHub;
    DetectorClass   & Detector;

    ASimulatorRunner * Runner;

    QThread simRunnerThread;
    QTimer simTimerGuiUpdate;

    int  MaxThreads = -1;
    bool fStartedFromGui = false;
    bool fHardAborted = false;
    bool fFinished;
    bool fSuccess;

public slots:
    void onSimulationFinished(); //processing of simulation results!
    void StopSimulation();

private slots:
    void onSimFailedToStart();    
    void updateGui();

signals:
    void updateReady(int Progress, double msPerEvent);
    void RequestStopSimulation();
    void SimulationFinished();

    void ProgressReport(int percents);
};

#endif // ASIMULATIONMANAGER_H
