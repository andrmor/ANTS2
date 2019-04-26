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
    ASimulationManager(EventsDataClass * EventsDataHub, DetectorClass * Detector);
    ~ASimulationManager();

    ASimulatorRunner * Runner;
    QThread simRunnerThread;
    QTimer simTimerGuiUpdate;

    bool fFinished;
    bool fSuccess;
    bool fHardAborted;

    bool fStartedFromGui;
    int LastSimType; // -1 - undefined, 0 - PointSources, 1 - ParticleSources

    int MaxThreads = -1;

    //last event info
    QVector< QBitArray > SiPMpixels;
    QVector<AEnergyDepositionCell *> EnergyVector;

    std::vector<TrackHolderClass *> Tracks;
    std::vector<ANodeRecord *> Nodes;

    void StartSimulation(QJsonObject & json, int threads, bool fStartedFromGui);

    void clearTracks();
    void clearNodes();
    void clearEnergyVector();

    const QString loadNodesFromFile(const QString & fileName);

    // Next three: Simulators use their own local copies constructed using configuration in JSON
    ASourceParticleGenerator * ParticleSources = 0;         //used to update JSON on config changes and in GUI to configure
    AFileParticleGenerator   * FileParticleGenerator = 0;   //only for gui, simulation threads use their own
    AScriptParticleGenerator * ScriptParticleGenerator = 0; //only for gui, simulation threads use their own

    ATrackBuildOptions TrackBuildOptions;

    //for G4ants sims
    QSet<QString> SeenNonRegisteredParticles;
    double DepoByNotRegistered;
    double DepoByRegistered;

private:
    EventsDataClass * EventsDataHub;    //external
    DetectorClass   * Detector;         //external

public slots:
    void onSimulationFinished(); //processing of simulation results!
    void StopSimulation();

private slots:
    void onSimFailedToStart();

signals:
    void RequestStopSimulation();
    void SimulationFinished();

    void ProgressReport(int percents);
};

#endif // ASIMULATIONMANAGER_H
