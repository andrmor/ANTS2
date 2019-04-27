#ifndef ASIMULATOR_H
#define ASIMULATOR_H

#include <QString>
#include <QSet>
#include <vector>

class DetectorClass;
class ASimulationManager;
class TrackHolderClass;
class AOneEvent;
class GeneralSimSettings;
class EventsDataClass;
class Photon_Generator;
class APhotonTracer;
class QJsonObject;
class TRandom2;

/******************************************************************************\
|                  Simulator base class (per-thread instance)                  |
\******************************************************************************/

class ASimulator
{
public:
    ASimulator(const DetectorClass * detector, ASimulationManager * simMan, const int ID);
    virtual ~ASimulator();

    const DetectorClass *getDetector() { return detector; }
    const QString getErrorString() const { return ErrorString; }
    virtual int getEventsDone() const = 0;

    char progress = 0;

    std::vector<TrackHolderClass *> tracks;  //temporary container for track data

    virtual int getEventCount() const = 0;
    virtual int getTotalEventCount() const = 0;
    const AOneEvent *getLastEvent() const { return OneEvent; }

    bool wasSuccessful() const { return (fSuccess && !fHardAbortWasTriggered); }
    bool wasHardAborted() const { return fHardAbortWasTriggered; }
    virtual void updateGeoManager();
    void setSimSettings(const GeneralSimSettings * settings);
    void initSimStat();
    void setRngSeed(int seed);
    void requestStop();

    void divideThreadWork(int threadId, int threadCount);
    virtual bool setup(QJsonObject & json);
    virtual void simulate() = 0;
    virtual void appendToDataHub(EventsDataClass * dataHub);

    virtual void hardAbort();

protected:
    virtual void ReserveSpace(int expectedNumEvents);
    int evenDivisionOfLabor(int totalEventCount);
    virtual void updateMaxTracks(int maxPhotonTracks, int maxParticleTracks);

    const DetectorClass *detector;          // external
    const ASimulationManager *simMan;       // external
    const GeneralSimSettings *simSettings;  // external
    TRandom2 *RandGen;                      // local
    AOneEvent* OneEvent;                    // local         //PM hit data for one event is stored here
    EventsDataClass *dataHub;               // local
    Photon_Generator *photonGenerator;      // local
    APhotonTracer* photonTracker;           // local

    QString ErrorString; //last error
    int ID;

    //state control
    int eventBegin = 0;
    int eventCurrent; //to be updated by implementor, or override getEventsDone()
    int eventEnd = 0;

    //control settings
    bool fUpdateGUI;
    bool fBuildPhotonTracks;
    bool fStopRequested; //Implementors must check whenever possible (without impacting performance) to stop simulation()
    bool fSuccess;  //Implementors should set this flag at end of simulation()
    bool fHardAbortWasTriggered;

    int maxPhotonTracks = 1000;
    int maxParticleTracks = 1000;

public:
    //for G4ants sim
    QSet<QString> SeenNonRegisteredParticles;
    double DepoByNotRegistered;
    double DepoByRegistered;
};

#endif // ASIMULATOR_H
