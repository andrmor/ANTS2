#ifndef SIMULATION_MANAGER_H
#define SIMULATION_MANAGER_H

#include "generalsimsettings.h"
#include "scanfloodstructure.h"
#include "aphoton.h"
#include "dotstgeostruct.h"

#include <QVector>
#include <QObject>
#include <QTime>
#include <QTimer>
#include <QThread>

#include "TString.h"
#include "TVector3.h"
#include "RVersion.h"

class DetectorClass;
class OneEventClass;
class EventsDataClass;
class AParticle;
class Photon_Generator;
class PrimaryParticleTracker;
class S1_Generator;
class S2_Generator;
class ParticleSourcesClass;
class Simulator;
class TRandom2;
class TString;
class TH1I;
class TGeoManager;
class APhotonTracer;
class TrackHolderClass;
struct AScanRecord;
class AParticleOnStack;
struct AEnergyDepositionCell;
class ASimulatorRunner;
class GeoMarkerClass;

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
    QThread simThread;
    QTimer simTimerGuiUpdate;

    bool fFinished;
    bool fSuccess;

    bool fStartedFromGui;
    int LastSimType; // -1 - undefined, 0 - PointSources, 1 - ParticleSources

    //info to report back
    APhoton LastPhoton;
    QVector< QBitArray > SiPMpixels;
    QVector<GeoMarkerClass*> GeoMarkers;
    QVector<AEnergyDepositionCell*> EnergyVector;
    QVector<TrackHolderClass*> Tracks;

    void StartSimulation(QJsonObject &json, int threads, bool fStartedFromGui);   
    void Clear();

    ParticleSourcesClass* ParticleSources;  //used to update JSON on config chamges and in GUI to configure; Simulateors use their loacl copies build from JSON

private:
    EventsDataClass* EventsDataHub; //alias
    DetectorClass* Detector; //alias

    void clearGeoMarkers();
    void clearEnergyVector();
    void clearTracks();

public slots:
    void onSimulationFinished(); //processing of simulation results!
    void StopSimulation();

signals:
    void RequestStopSimulation();
    void SimulationFinished();

};

class ASimulatorRunner : public QObject
{
    Q_OBJECT
public:
    enum State { SClean, SSetup, SRunning, SFinished/*, SStopRequest*/ };
    QString modeSetup;

    explicit ASimulatorRunner(DetectorClass *detector, EventsDataClass *dataHub, QObject *parent = 0);
    virtual ~ASimulatorRunner();

    void setup(QJsonObject &json, int threadCount);
    void updateGeoManager();
    void setWorkersSeed(int rngSeed); //even with same seed, threadCount must be the same for same results!!!
    bool getStoppedByUser() const { return fStopRequested; /*simState == SStopRequest;*/ }
    void updateStats();
    double getProgress() const { return progress; }
    //double getmsPerEvent() const { return usPerEvent; }
    bool wasSuccessful() const;
    bool isFinished() const {return simState == SFinished;}
    QString getErrorMessages() const;
    //Use as read-only. Anything else is undefined behaviour! If your toast gets burnt, it's not my fault!
    //Also remember that simulators will be deleted on setup()!
    QVector<Simulator *> getWorkers() { return workers; }
    //const Simulator *getLastWorker() const; //Not needed since we create only the necessary workers

    //Use this with caution too, e.g. after simulation finished! We need to expose this to clear memory after simulation,
    //since we provide extra information of simulators to the outside (and it's actually used). In a way this is a hack
    //that should be improved, maybe through external container (EventDataHub is not enough or suitable for this)
    void clearWorkers();

private:
    //void initQEAccelerator(); //configures MaxQE and MaxQEvsWave

    //Manager's state
    enum State simState;
    bool fStopRequested;
    //TRandom2 *randGen;

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
    Simulator(const DetectorClass *detector, const TString &nameID);
    virtual ~Simulator();

    const DetectorClass *getDetector() { return detector; }
    QString getErrorString() const { return ErrorString; }
    virtual int getEventsDone() const = 0;

    char progress;

    QVector<TrackHolderClass*> tracks;  //temporary container for tracks, they are transferred (amd merged) to GeoManager after simulation is finished

    virtual int getEventCount() const = 0;
    virtual int getTotalEventCount() const = 0;
    const OneEventClass *getLastEvent() const { return OneEvent; }

    bool wasSuccessful() const { return fSuccess; }
    virtual void updateGeoManager();
    void setSimSettings(const GeneralSimSettings *settings);
    void initSimStat();
    void setRngSeed(int seed);
    void requestStop();

    void divideThreadWork(int threadId, int threadCount);
    virtual bool setup(QJsonObject &json);
    virtual void simulate() = 0;
    virtual void appendToDataHub(EventsDataClass *dataHub);

    TString getNameId() const {return nameID;}

protected:
    virtual void ReserveSpace(int expectedNumEvents);
    int evenDivisionOfLabor(int totalEventCount);

    const DetectorClass *detector;
    TRandom2 *RandGen;
    OneEventClass* OneEvent; //PM hit data for one event is stored here
    EventsDataClass *dataHub;
    Photon_Generator *photonGenerator;
    QString ErrorString; //last error
    TString nameID;

    APhotonTracer* photonTracker;

    //state control
    int eventBegin;
    int eventCurrent; //to be updated by implementor, or override getEventsDone()
    int eventEnd;

    //control settings
    bool fUpdateGUI;
    bool fBuildPhotonTracks;
    bool fStopRequested; //Implementors must check whenever possible (without impacting performance) to stop simulation()
    bool fSuccess;  //Implementors should set this flag at end of simulation()

    //general simulation options
    const GeneralSimSettings *simSettings;

private:
};

class PointSourceSimulator : public Simulator
{
public:
    explicit PointSourceSimulator(const DetectorClass *detector, const TString &nameID);
    ~PointSourceSimulator();

    virtual int getEventCount() const;
    virtual int getTotalEventCount() const { return totalEventCount; }
    virtual int getEventsDone() const { return eventCurrent; }
    virtual bool setup(QJsonObject &json);
    virtual void simulate();
    virtual void appendToDataHub(EventsDataClass *dataHub);

    const QVector<DotsTGeoStruct> *getDotsTGeo() const { return DotsTGeo; }
    const APhoton *getLastPhotonOnStart() const { return &PhotonOnStart; }
    int getNumRuns() const {return NumRuns;}

private:
    bool SimulateSingle();
    bool SimulateRegularGrid();
    bool SimulateFlood();
    bool SimulateCustomNodes();

    //utilities
    bool ReadPhotonSimOptions(QJsonObject &json);
    int PhotonsToRun();
    void GenerateTraceNphotons(AScanRecord *scs, int iPoint = 0);
    bool FindSecScintBounds(double *r, double *z1, double *z2);
    void OneNode(double *r);
    void doLRFsimulatedEvent(double *r);
    void GenerateFromSecond(AScanRecord *scs);
    bool isInsideLimitingObject(double *r);
    virtual void ReserveSpace(int expectedNumEvents);
    void addLastScanPointToMarkers(bool fLimitNumber = true);

    QJsonObject simOptions;
    TH1I *CustomHist; //custom photon generation distribution
    QVector<DotsTGeoStruct> *DotsTGeo;

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
    bool fDirectGeneration;
    double iWaveIndex;
    double DecayTime;
    int iMatIndex;

    //bad events options
    QVector<ScanFloodStructure> BadEventConfig;
    bool fBadEventsOn;
    double BadEventTotalProbability;
    double SigmaDouble; //Gaussian distribution is used to position second event
};

class ParticleSourceSimulator : public Simulator
{
public:
    explicit ParticleSourceSimulator(const DetectorClass *detector, const TString &nameID);
    ~ParticleSourceSimulator();

    const QVector<AEnergyDepositionCell*> &getEnergyVector() const { return EnergyVector; }
    void ClearEnergyVector() {EnergyVector.resize(0);} //to avoid clear of objects stored in the vector

    virtual int getEventCount() const { return eventEnd - eventBegin; }
    virtual int getEventsDone() const { return eventCurrent - eventBegin; }
    virtual int getTotalEventCount() const { return totalEventCount; }
    virtual bool setup(QJsonObject &json);
    virtual void updateGeoManager();
    virtual void simulate();
    virtual void appendToDataHub(EventsDataClass *dataHub);

    //test purposes - direct tracking with provided stack or photon generation from provided energy deposition 
    bool standaloneTrackStack(QVector<AParticleOnStack*>* particleStack);
    bool standaloneGenerateLight(QVector<AEnergyDepositionCell*>* energyVector);

private:
    //utilities
    void EnergyVectorToScan();
    void clearParticleStack();

    //local objects
    PrimaryParticleTracker* ParticleTracker;
    S1_Generator* S1generator;
    S2_Generator* S2generator;
    ParticleSourcesClass *ParticleSources;
    QVector<AEnergyDepositionCell*> EnergyVector;
    QVector<AParticleOnStack*> ParticleStack;

    int totalEventCount;
    double timeFrom, timeRange;

    //Control
    bool fBuildParticleTracks;    
    bool fDoS1;
    bool fDoS2;
    bool fAllowMultiple; //multiple particles per event?
    int AverageNumParticlesPerEvent;
    int TypeParticlesPerEvent;  //0 - constant, 1 - Poisson
    bool fIgnoreNoHitsEvents;
    bool fIgnoreNoDepoEvents;
};

#endif // SIMULATION_MANAGER_H
