#ifndef APARTICLETRACKER_H
#define APARTICLETRACKER_H

#include <QVector>
#include <vector>

class TRandom2;
class AParticle;
class AMaterial;
class AParticleRecord;
class AEventTrackingRecord;
struct AEnergyDepositionCell;
class AMaterialParticleCollection;
class GeneralSimSettings;
class TrackHolderClass;
class ASimulationStatistics;
class AEventTrackingRecord;
class AParticleTrackingRecord;
class TGeoNavigator;
struct MatParticleStructure;
struct NeutralTerminatorStructure;

class AParticleTracker
{
public:
    explicit AParticleTracker(TRandom2 & RandomGenerator,
                              AMaterialParticleCollection & MpCollection,
                              QVector<AParticleRecord*> & particleStack,
                              QVector<AEnergyDepositionCell*> & energyVector,
                              std::vector<AEventTrackingRecord *> & TrackingHistory,
                              ASimulationStatistics & simStat,
                              int ThreadIndex);

    bool TrackParticlesOnStack(int eventId = 0);

    void configure(const GeneralSimSettings *simSet,
                   bool fbuildTrackes,
                   std::vector<TrackHolderClass *> * tracks,
                   bool fRemoveEmptyTracks = true);


    void resetCounter() {counter = -1;}
    void setMaxTracks(int maxTracks) {MaxTracks = maxTracks;}

private:
    TRandom2 & RandGen;
    AMaterialParticleCollection & MpCollection;
    QVector<AParticleRecord*> & ParticleStack;   // TODO --> std::vector
    QVector<AEnergyDepositionCell*> & EnergyVector; // TODO --> std::vector
    std::vector<AEventTrackingRecord *> & TrackingHistory;
    ASimulationStatistics & SimStat;
    int ThreadIndex = 0;

    const GeneralSimSettings* SimSet;
    bool BuildTracks = false;
    int MaxTracks = 10;
    int ParticleTracksAdded = 0;
    bool RemoveTracksIfNoEnergyDepo = true;
    std::vector<TrackHolderClass *> TrackCandidates;
    std::vector<TrackHolderClass *> * Tracks;

    int counter = -1;          //particle "serial" number - can be reset from outside  // obsolete?
    int EventId;

    // --- runtime ---

    AParticleRecord * p = nullptr; //current particle

    int thisMatId;
    AMaterial * thisMaterial = nullptr;
    MatParticleStructure * thisMatParticle = nullptr;

    TGeoNavigator * navigator = nullptr;

    TrackHolderClass * track = nullptr;
    AEventTrackingRecord * EventRecord = nullptr;
    AParticleTrackingRecord * thisParticleRecord = nullptr;

    bool bBuildThisTrack = false;

    void generateRandomDirection(double* vv);
    void initLog();
    void initTrack();

    bool checkMonitors_isKilled(); // return true if the particle is stopped by monitor

    bool trackCharged_isKilled();
    bool trackNeutral_isKilled();

    bool processPhotoelectric_isKilled();
    bool processCompton_isKilled();
    bool processNeutronAbsorption_isKilled(const NeutralTerminatorStructure & term);
    bool processPairProduction_isKilled();
    bool processNeutronElastic_isKilled(const NeutralTerminatorStructure & term);
};

#endif // APARTICLETRACKER_H
