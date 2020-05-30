#ifndef ASIMULATOR_H
#define ASIMULATOR_H

#include <QString>
#include <QSet>
#include <vector>

class DetectorClass;
class ASimulationManager;
class TrackHolderClass;
class AOneEvent;
class AGeneralSimSettings;
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
    ASimulator(ASimulationManager & simMan, int ID);
    virtual ~ASimulator();

    const QString getErrorString() const { return ErrorString; }
    virtual int getEventsDone() const = 0;
    int getTreadId() const {return ID;}

    int progress = 0; // progress in percents
    int progressG4 = 0; // progress of G4ants sim in percents

    std::vector<TrackHolderClass *> tracks;  //temporary container for track data

    virtual int getEventCount() const = 0;
    virtual int getTotalEventCount() const = 0;
    const AOneEvent *getLastEvent() const { return OneEvent; }

    bool wasSuccessful() const { return (fSuccess && !fHardAbortWasTriggered); }
    bool wasHardAborted() const { return fHardAbortWasTriggered; }
    virtual void updateGeoManager();
    void initSimStat();
    void requestStop();

    void divideThreadWork(int threadId, int threadCount);
    virtual bool setup(QJsonObject & json);
    virtual bool finalizeConfig() {return true;} //called after setup and divide work
    virtual void simulate() = 0;
    virtual void appendToDataHub(EventsDataClass * dataHub);
    virtual void mergeData() = 0;

    virtual void hardAbort();

protected:
    virtual void ReserveSpace(int expectedNumEvents);
    int evenDivisionOfLabor(int totalEventCount);
    virtual void updateMaxTracks(int maxPhotonTracks, int maxParticleTracks);

    ASimulationManager & simMan;
    int ID;

    const DetectorClass & detector;
    const AGeneralSimSettings & simSettings;

    // local resources
    TRandom2 *RandGen;
    AOneEvent* OneEvent; //PM hit data for one event is stored here
    EventsDataClass *dataHub;
    Photon_Generator *photonGenerator;
    APhotonTracer* photonTracker;

    QString ErrorString; //last error

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

protected:
    void checkNavigatorPresent();

};

#endif // ASIMULATOR_H
