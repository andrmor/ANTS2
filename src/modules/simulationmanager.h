#ifndef SIMULATION_MANAGER_H
#define SIMULATION_MANAGER_H

#include "generalsimsettings.h"
#include "scanfloodstructure.h"
#include "aphoton.h"
#include "dotstgeostruct.h"
#include "atrackbuildoptions.h"

#include <QVector>
#include <vector>
#include <QObject>
#include <QTime>
#include <QTimer>
#include <QThread>

#include "TString.h"
#include "TVector3.h"
#include "RVersion.h"

class DetectorClass;
class AOneEvent;
class EventsDataClass;
class AParticle;
class Photon_Generator;
class PrimaryParticleTracker;
class S1_Generator;
class S2_Generator;
class ASourceParticleGenerator;
class AFileParticleGenerator;
class AScriptParticleGenerator;
class Simulator;
class TRandom2;
class TString;
class TH1I;
class TGeoManager;
class APhotonTracer;
class TrackHolderClass;
struct AScanRecord;
class AParticleRecord;
struct AEnergyDepositionCell;
class ASimulatorRunner;
class GeoMarkerClass;
class AParticleGun;
class QProcess;

#if ROOT_VERSION_CODE < ROOT_VERSION(6,11,1)
class TThread;
#else
namespace std { class thread;}
#endif


class ASimulationManager : public QObject
{
    Q_OBJECT
public:
    ASimulationManager(EventsDataClass* EventsDataHub, DetectorClass* Detector);
    ~ASimulationManager();

    ASimulatorRunner* Runner;
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
    QVector<AEnergyDepositionCell*> EnergyVector;

    std::vector<TrackHolderClass *> Tracks;

    void StartSimulation(QJsonObject &json, int threads, bool fStartedFromGui);

    void clearTracks();

    // Next three: Simulators use their own local copies constructed using configuration in JSON
    ASourceParticleGenerator*     ParticleSources = 0;         //used to update JSON on config changes and in GUI to configure
    AFileParticleGenerator*   FileParticleGenerator = 0;   //only for gui, simulation threads use their own
    AScriptParticleGenerator* ScriptParticleGenerator = 0; //only for gui, simulation threads use their own

    ATrackBuildOptions TrackBuildOptions;

private:
    EventsDataClass* EventsDataHub; //alias
    DetectorClass* Detector; //alias

    void clearEnergyVector();

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

class ASimulatorRunner : public QObject
{
    Q_OBJECT
public:
    enum State { SClean, SSetup, SRunning, SFinished/*, SStopRequest*/ };
    QString modeSetup;

    explicit ASimulatorRunner(DetectorClass *detector, EventsDataClass *dataHub, QObject *parent = 0);
    virtual ~ASimulatorRunner();

    bool setup(QJsonObject &json, int threadCount, bool bFromGui);
    void updateGeoManager();
    bool getStoppedByUser() const { return fStopRequested; /*simState == SStopRequest;*/ }
    void updateStats();
    double getProgress() const { return progress; }
    bool wasSuccessful() const;
    bool wasHardAborted() const;
    bool isFinished() const {return simState == SFinished;}
    void setFinished() {simState = SFinished;}
    QString getErrorMessages() const;
    //Use as read-only. Anything else is undefined behaviour! If your toast gets burnt, it's not my fault!
    //Also remember that simulators will be deleted on setup()!
    QVector<Simulator *> getWorkers() { return workers; }

    //Use this with caution too, e.g. after simulation finished! We need to expose this to clear memory after simulation,
    //since we provide extra information of simulators to the outside (and it's actually used). In a way this is a hack
    //that should be improved, maybe through external container (EventDataHub is not enough or suitable for this)
    void clearWorkers();

    void setG4Sim() {bNextSimExternal = true;}
    void setG4Sim_OnlyGenerateFiles(bool flag) {bOnlyFileExport = flag;}

private:
    //void initQEAccelerator(); //configures MaxQE and MaxQEvsWave

    //Manager's state
    enum State simState;
    bool fStopRequested;
    bool bRunFromGui;

    //Threads
    QVector<Simulator *> workers;
#if ROOT_VERSION_CODE < ROOT_VERSION(6,11,1)
    QVector<TThread *> threads;
#else
    QVector<std::thread *> threads;
#endif
    Simulator *backgroundWorker;

    //External settings
    DetectorClass *detector;
    EventsDataClass *dataHub;
    GeneralSimSettings simSettings;

    //Time
    QTime startTime;
    int lastRefreshTime;
    int totalEventCount;
    int lastProgress;
    int lastEventsDone;

    //Stats
    double progress;
    double usPerEvent;

    QString ErrorString;

    bool bNextSimExternal = false;
    bool bOnlyFileExport = false;

public slots:
    void simulate();
    void requestStop();
    void updateGui();

signals:
    void updateReady(int Progress, double msPerEvent);
    void simulationFinished();
};

class Simulator
{
public:
    Simulator(const DetectorClass *detector, const int ID);
    virtual ~Simulator();

    const DetectorClass *getDetector() { return detector; }
    const QString getErrorString() const { return ErrorString; }
    virtual int getEventsDone() const = 0;

    char progress;

    std::vector<TrackHolderClass *> tracks;  //temporary container for track data

    virtual int getEventCount() const = 0;
    virtual int getTotalEventCount() const = 0;
    const AOneEvent *getLastEvent() const { return OneEvent; }

    bool wasSuccessful() const { return (fSuccess && !fHardAbortWasTriggered); }
    bool wasHardAborted() const { return fHardAbortWasTriggered; }
    virtual void updateGeoManager();
    void setSimSettings(const GeneralSimSettings *settings);
    void initSimStat();
    void setRngSeed(int seed);
    void requestStop();

    void divideThreadWork(int threadId, int threadCount);
    virtual bool setup(QJsonObject &json);
    virtual void simulate() = 0;
    virtual void appendToDataHub(EventsDataClass *dataHub);

    virtual void hardAbort();

protected:
    virtual void ReserveSpace(int expectedNumEvents);
    int evenDivisionOfLabor(int totalEventCount);
    virtual void updateMaxTracks(int maxPhotonTracks, int maxParticleTracks);

    const DetectorClass *detector;          // external
    const GeneralSimSettings *simSettings;  // external
    TRandom2 *RandGen;                      // local
    AOneEvent* OneEvent;                    // local         //PM hit data for one event is stored here
    EventsDataClass *dataHub;               // local
    Photon_Generator *photonGenerator;      // local
    APhotonTracer* photonTracker;           // local

    QString ErrorString; //last error
    int ID;

    //state control
    int eventBegin;
    int eventCurrent; //to be updated by implementor, or override getEventsDone()
    int eventEnd;

    //control settings
    bool fUpdateGUI;
    bool fBuildPhotonTracks;
    bool fStopRequested; //Implementors must check whenever possible (without impacting performance) to stop simulation()
    bool fSuccess;  //Implementors should set this flag at end of simulation()
    bool fHardAbortWasTriggered;

private:
};

class PointSourceSimulator : public Simulator
{
public:
    explicit PointSourceSimulator(const DetectorClass *detector, int ID);
    ~PointSourceSimulator();

    virtual int getEventCount() const;
    virtual int getTotalEventCount() const { return totalEventCount; }
    virtual int getEventsDone() const { return eventCurrent; }
    virtual bool setup(QJsonObject &json);
    virtual void simulate();
    virtual void appendToDataHub(EventsDataClass *dataHub);

    int getNumRuns() const {return NumRuns;}

private:
    bool SimulateSingle();
    bool SimulateRegularGrid();
    bool SimulateFlood();
    bool SimulateCustomNodes();

    //utilities
    int  PhotonsToRun();
    void GenerateTraceNphotons(AScanRecord *scs, double time0 = 0, int iPoint = 0);
    bool FindSecScintBounds(double *r, double & z1, double & z2, double & timeOfDrift, double &driftSpeedInSecScint);
    void OneNode(double *r, double time0 = 0);
    void doLRFsimulatedEvent(double *r);
    void GenerateFromSecond(AScanRecord *scs, double time0);
    bool isInsideLimitingObject(double *r);
    virtual void ReserveSpace(int expectedNumEvents);

    QJsonObject simOptions;
    TH1I *CustomHist; //custom photon generation distribution

    APhoton PhotonOnStart; //properties of the photon which are used to initiate Photon_Tracker

    int totalEventCount;

    int PointSimMode;            // 0-Single 1-Scan 2-Flood    
    int ScintType;               // 1 - primary, 2 - secondary
    int NumRuns;                 // multiple runs per node

    //bool fOnlyPrimScint; //do not create event outside of prim scintillator
    bool fLimitNodesToObject;
    TString LimitNodesToObject;

    //photons per node info
    int numPhotMode; // 0-constant, 1-uniform, 2-gauss, 3-custom
    int numPhotsConst;
    int numPhotUniMin, numPhotUniMax;
    double numPhotGaussMean, numPhotGaussSigma;

    //photon direction option
    bool fRandomDirection;
    //direction vector is in PhotonOnStart
    TVector3 ConeDir;
    double CosConeAngle;
    bool fCone;

    //wavelength and time options
    bool fUseGivenWaveIndex;
    //bool fAutomaticWaveTime = false;
    double iFixedWaveIndex;
    //double DecayTime;
    //int iMatIndex;

    //bad events options
    QVector<ScanFloodStructure> BadEventConfig;
    bool fBadEventsOn;
    double BadEventTotalProbability;
    double SigmaDouble; //Gaussian distribution is used to position second event

};

class ParticleSourceSimulator : public Simulator
{
public:
    explicit ParticleSourceSimulator(const DetectorClass *detector, int ID);
    ~ParticleSourceSimulator();

    const QVector<AEnergyDepositionCell*> &getEnergyVector() const { return EnergyVector; }
    void ClearEnergyVectorButKeepObjects() {EnergyVector.resize(0);} //to avoid clear of objects stored in the vector

    virtual int getEventCount() const { return eventEnd - eventBegin; }
    virtual int getEventsDone() const { return eventCurrent - eventBegin; }
    virtual int getTotalEventCount() const { return totalEventCount; }
    virtual bool setup(QJsonObject &json);
    virtual void updateGeoManager();
    virtual void simulate();
    virtual void appendToDataHub(EventsDataClass *dataHub);

    //test purposes - direct tracking with provided stack or photon generation from provided energy deposition 
    bool standaloneTrackStack(QVector<AParticleRecord*>* particleStack);
    bool standaloneGenerateLight(QVector<AEnergyDepositionCell*>* energyVector);

    void setOnlySavePrimaries() {bOnlySavePrimariesToFile = true;}

    virtual void hardAbort() override;

protected:
    virtual void updateMaxTracks(int maxPhotonTracks, int maxParticleTracks);

private:
    void EnergyVectorToScan();
    void clearParticleStack();
    void clearEnergyVector();
    void clearGeneratedParticles();

    int  chooseNumberOfParticlesThisEvent() const;
    bool choosePrimariesForThisEvent(int numPrimaries);
    bool generateAndTrackPhotons();
    bool geant4TrackAndProcess();

    //local objects
    PrimaryParticleTracker* ParticleTracker = 0;
    S1_Generator* S1generator = 0;
    S2_Generator* S2generator = 0;
    AParticleGun* ParticleGun = 0;
    QVector<AEnergyDepositionCell*> EnergyVector;
    QVector<AParticleRecord*> ParticleStack;

    //local use - container which particle generator fills for each event; the particles are deleted by the tracker
    QVector<AParticleRecord*> GeneratedParticles;

    int totalEventCount;
    double timeFrom, timeRange;
    double updateFactor;

    //Control
    bool fBuildParticleTracks;   //can be dropped and use directly TrackBuildOptions od simSettings
    bool fDoS1;
    bool fDoS2;
    bool fAllowMultiple; //multiple particles per event?
    int AverageNumParticlesPerEvent;
    int TypeParticlesPerEvent;  //0 - constant, 1 - Poisson
    bool fIgnoreNoHitsEvents;
    bool fIgnoreNoDepoEvents;
    double ClusterMergeRadius2 = 1.0; //scan cluster merge radius [mm] in square - used by EnergyVectorToScan()

    //Geant4 interface
    bool bOnlySavePrimariesToFile = false;
    QProcess * G4antsProcess = 0;
    bool bG4isRunning = false;
};

#endif // SIMULATION_MANAGER_H
