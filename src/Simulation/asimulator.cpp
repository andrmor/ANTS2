#include "asimulator.h"
#include "detectorclass.h"
#include "asimulationmanager.h"
#include "atrackrecords.h"
#include "aoneevent.h"
#include "ageneralsimsettings.h"
#include "eventsdataclass.h"
#include "photon_generator.h"
#include "aphotontracer.h"
#include "asandwich.h"

#include <QDebug>
#include <QJsonObject>

#include "TRandom2.h"
#include "TGeoManager.h" //to move?

ASimulator::ASimulator(ASimulationManager &simMan, int threadID) :
    simMan(simMan), ID(threadID),
    detector(simMan.getDetector()), GenSimSettings(simMan.Settings.genSimSet)
{
    RandGen = new TRandom2();
    int seed = detector.RandGen->Rndm() * 10000000;
    RandGen->SetSeed(seed);

    dataHub = new EventsDataClass(threadID);
    OneEvent = new AOneEvent(detector.PMs, RandGen, dataHub->SimStat);
    photonGenerator = new Photon_Generator(detector, *RandGen);
    photonTracker = new APhotonTracer(detector.GeoManager, RandGen, detector.MpCollection, detector.PMs, &detector.Sandwich->GridRecords);
}

ASimulator::~ASimulator()
{
    delete photonTracker;
    delete photonGenerator;
    delete dataHub;
    delete OneEvent;
    delete RandGen;

    //if transferred, the container will be cleared, need cleaning only in case of abort
    for (TrackHolderClass* t : tracks) delete t;
    tracks.clear();
}

void ASimulator::updateGeoManager()
{
    photonTracker->UpdateGeoManager(detector.GeoManager);
}

void ASimulator::initSimStat()
{
    dataHub->initializeSimStat(detector.Sandwich->MonitorsRecords, GenSimSettings.DetStatNumBins, (GenSimSettings.fWaveResolved ? GenSimSettings.WaveNodes : 0));
}

void ASimulator::requestStop()
{
    if (fStopRequested)
    {
        //  qDebug() << "Simulator #" << ID << "recived repeated stop request, aborting in hard mode";
        hardAbort();
        fHardAbortWasTriggered = true;
    }
    else
    {
        //  qDebug() << "First stop request was received by simulator #"<<ID;
        fStopRequested = true;
    }
}

void ASimulator::divideThreadWork(int threadId, int threadCount)
{
    //  qDebug() <<  "This thread:" << threadId << "Count treads:"<<threadCount;
    int totalEventCount = getTotalEventCount();
    int nodesPerThread = totalEventCount / threadCount;
    int remainingNodes = totalEventCount % threadCount;
    //The first <remainingNodes> threads get an extra node each
    eventBegin = threadId*nodesPerThread + std::min(threadId, remainingNodes);
    eventEnd = eventBegin + nodesPerThread + (threadId >= remainingNodes ? 0 : 1);
    //  qDebug()<<"Thread"<<threadId<<"/"<<threadCount<<": eventBegin"<<eventBegin<<": eventEnd"<<eventEnd<<": eventCount"<<getEventCount()<<"/"<<getTotalEventCount();

    //dividing track building
    if (GenSimSettings.TrackBuildOptions.bBuildPhotonTracks)
        maxPhotonTracks = ceil( 1.0 * getEventCount() / getTotalEventCount() * GenSimSettings.TrackBuildOptions.MaxPhotonTracks );
    else maxPhotonTracks = 0;
    if (GenSimSettings.TrackBuildOptions.bBuildParticleTracks)
        maxParticleTracks = ceil( 1.0 * getEventCount() / getTotalEventCount() * GenSimSettings.TrackBuildOptions.MaxParticleTracks );
    else maxParticleTracks = 0;
    updateMaxTracks(maxPhotonTracks, maxParticleTracks);
    //  qDebug() << maxPhotonTracks << maxParticleTracks;
}

bool ASimulator::setup()
{
    dataHub->clear();

    OneEvent->configure(&GenSimSettings);
    photonGenerator->configure(&GenSimSettings, OneEvent->SimStat);
    photonTracker->configure(&GenSimSettings, OneEvent, GenSimSettings.TrackBuildOptions.bBuildPhotonTracks, &tracks);
    return true;
}

void ASimulator::appendToDataHub(EventsDataClass *dataHub)
{
    dataHub->Events << this->dataHub->Events; //static
    dataHub->TimedEvents << this->dataHub->TimedEvents; //static
    dataHub->Scan << this->dataHub->Scan; //dynamic!
    this->dataHub->Scan.clear();

    dataHub->SimStat->AppendSimulationStatistics(this->dataHub->SimStat); //deep copy
}

void ASimulator::hardAbort()
{
    photonTracker->hardAbort();
}

void ASimulator::ReserveSpace(int expectedNumEvents)
{
    dataHub->Events.reserve(expectedNumEvents);
    if (GenSimSettings.fTimeResolved) dataHub->TimedEvents.reserve(expectedNumEvents);
    dataHub->Scan.reserve(expectedNumEvents);
}

void ASimulator::updateMaxTracks(int maxPhotonTracks, int /*maxParticleTracks*/)
{
    photonTracker->setMaxTracks(maxPhotonTracks);
}

void ASimulator::assureNavigatorPresent()
{
    // it is normal to be triggered once per tread on sim start!
    if (!detector.GeoManager->GetCurrentNavigator())
    {
        //qDebug() << "No current navigator for this thread, adding one";
        detector.GeoManager->AddNavigator();
    }
}
