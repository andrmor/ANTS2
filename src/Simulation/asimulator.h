#ifndef ASIMULATOR_H
#define ASIMULATOR_H

#include <QString>
#include <QSet>
#include <vector>

class ASimSettings;
class DetectorClass;
class TrackHolderClass;
class AOneEvent;
class AGeneralSimSettings;
class EventsDataClass;
class Photon_Generator;
class APhotonTracer;
class TRandom2;

// tread worker for simulation - base class
class ASimulator
{
public:
    ASimulator(const ASimSettings & simSet, const DetectorClass & detector, int threadIndex, int startSeed);
    virtual ~ASimulator();

    virtual int getEventsDone() const = 0;
    virtual int getEventCount() const = 0;
    virtual int getTotalEventCount() const = 0;
    virtual void updateGeoManager();             // obsolete - remove
    virtual bool setup();
    virtual bool finalizeConfig() {return true;} // called after setup and divide work
    virtual void simulate() = 0;
    virtual void appendToDataHub(EventsDataClass * dataHub);
    virtual void hardAbort();

    const QString getErrorString() const {return ErrorString;}
    int getTreadId() const {return ThreadIndex;}

    const AOneEvent * getLastEvent() const {return OneEvent;}

    bool wasSuccessful()  const {return (fSuccess && !fHardAbortWasTriggered);}
    bool wasHardAborted() const {return fHardAbortWasTriggered;}
    void initSimStat();
    void requestStop();

    void divideThreadWork(int threadId, int threadCount);

    int progress   = 0;   // progress in percents
    int progressG4 = 0; // progress of G4ants sim in percents

    std::vector<TrackHolderClass *> tracks;  //temporary container for track data

protected:
    virtual void ReserveSpace(int expectedNumEvents);
    virtual void updateMaxTracks(int maxPhotonTracks, int maxParticleTracks);

    int evenDivisionOfLabor(int totalEventCount);

    const ASimSettings        & SimSet;
    const AGeneralSimSettings & GenSimSettings;
    const DetectorClass       & detector;
    int                         ThreadIndex;

    // local resources
    TRandom2         * RandGen  = nullptr;
    AOneEvent        * OneEvent = nullptr; //PM hit data for one event is stored here
    EventsDataClass  * dataHub = nullptr;
    Photon_Generator * photonGenerator = nullptr;
    APhotonTracer    * photonTracker = nullptr;

    QString ErrorString; //last error

    //state control
    int eventBegin = 0;
    int eventCurrent; //to be updated by implementor, or override getEventsDone()
    int eventEnd = 0;

    //control settings
    bool fStopRequested; // Implementors must check whenever possible (without impacting performance) to stop simulation()
    bool fSuccess;       // Implementors should set this flag at end of simulation()
    bool fHardAbortWasTriggered;

    int maxPhotonTracks = 1000;    // TODO  !*! check it is updated from new track settings!
    int maxParticleTracks = 1000;  // TODO  !*! check it is updated from new track settings!

protected:
    void assureNavigatorPresent();

};

#endif // ASIMULATOR_H
