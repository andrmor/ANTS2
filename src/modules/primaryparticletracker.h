#ifndef PRIMARYPARTICLETRACKER_H
#define PRIMARYPARTICLETRACKER_H

#include <QObject>
#include <QVector>

class TGeoManager;
class TRandom2;
class AParticle;
class AMaterial;
class AParticleOnStack;
struct AEnergyDepositionCell;
struct EventHistoryStructure;
class AMaterialParticleCollection;
class TVirtualGeoTrack;
class GeneralSimSettings;
class TrackHolderClass;
class ASimulationStatistics;

class PrimaryParticleTracker : public QObject
{
    Q_OBJECT
public:
    explicit PrimaryParticleTracker(TGeoManager* geoManager,
                                    TRandom2* RandomGenerator,
                                    AMaterialParticleCollection* MpCollection,
                                    QVector<AParticleOnStack*>* particleStack,
                                    QVector<AEnergyDepositionCell*>* energyVector,
                                    QVector<EventHistoryStructure*>* eventHistory,
                                    ASimulationStatistics* simStat,
                                    int threadIndex,
                                    QObject *parent = 0);
    //main usage
    bool TrackParticlesOnStack(int eventId = 0);

    //configure
    void setBuildTracks(bool opt){ BuildTracks = opt;}
    void setRemoveTracksIfNoEnergyDepo(bool opt){ RemoveTracksIfNoEnergyDepo = opt;}
      //configure all
    void configure(const GeneralSimSettings *simSet,                 
                   bool fbuildTrackes,
                   QVector<TrackHolderClass*> *tracks,
                   bool fremoveEmptyTracks = true);


    void UpdateGeoManager(TGeoManager* NewGeoManager) {GeoManager = NewGeoManager;} //will be absolete
    void resetCounter() {counter = -1;}  //will be absolute

signals:
    
public slots:

private:   
    TGeoManager* GeoManager;
    TRandom2* RandGen;
    AMaterialParticleCollection* MpCollection;
    QVector<AParticleOnStack*>* ParticleStack;
    QVector<AEnergyDepositionCell*>* EnergyVector;
    QVector<EventHistoryStructure*>* EventHistory;
    ASimulationStatistics* SimStat;

    int threadIndex = 0;

    const GeneralSimSettings* SimSet;
    bool BuildTracks;
    bool RemoveTracksIfNoEnergyDepo;
    QVector<TrackHolderClass*> TrackCandidates;
    QVector<TrackHolderClass*> *Tracks;

    //int Id;
    //double energy, time;
    //double r[3];          //position
    //double v[3];          //direction vector

    int counter;          //particle "serial" number - can be reset from outside

    void GenerateRandomDirection(double* vv);
};

#endif // PRIMARYPARTICLETRACKER_H
