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

class AParticleTracker
{
public:
    explicit AParticleTracker(TRandom2 & RandomGenerator,
                              AMaterialParticleCollection & MpCollection,
                              QVector<AParticleRecord*> & particleStack,
                              std::vector<AEventTrackingRecord *> & TrackingHistory,
                              ASimulationStatistics & simStat,
                              int ThreadIndex);

    ~AParticleTracker();

    //main usage
    bool TrackParticlesOnStack(int eventId = 0);

    //configure
    void setBuildTracks(bool opt){ BuildTracks = opt;}  // to remove
    void setRemoveTracksIfNoEnergyDepo(bool opt){ RemoveTracksIfNoEnergyDepo = opt;} // to remove
      //configure all
    void configure(const GeneralSimSettings *simSet,
                   bool fbuildTrackes,
                   std::vector<TrackHolderClass *> * tracks,
                   bool fremoveEmptyTracks = true);


    void resetCounter() {counter = -1;}  //will be absolute
    void setMaxTracks(int maxTracks) {MaxTracks = maxTracks;}

private:
    TRandom2 & RandGen;
    AMaterialParticleCollection & MpCollection;
    QVector<AParticleRecord*> & ParticleStack;   // TODO --> std::vector
    std::vector<AEventTrackingRecord *> & TrackingHistory;
    ASimulationStatistics & SimStat;
    int ThreadIndex = 0;

    std::vector<AEnergyDepositionCell*> EnergyVector;

    const GeneralSimSettings* SimSet;
    bool BuildTracks = false;
    int MaxTracks = 10;
    int ParticleTracksAdded = 0;
    bool RemoveTracksIfNoEnergyDepo = true;
    std::vector<TrackHolderClass *> TrackCandidates;
    std::vector<TrackHolderClass *> * Tracks;

    int counter = -1;          //particle "serial" number - can be reset from outside  // obsolete!

    void generateRandomDirection(double* vv);
    void clearEnergyVector();
};

#endif // APARTICLETRACKER_H
