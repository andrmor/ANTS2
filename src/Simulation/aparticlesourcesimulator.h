#ifndef APARTICLESOURCESIMULATOR_H
#define APARTICLESOURCESIMULATOR_H

#include "asimulator.h"

#include <QVector>

class AEnergyDepositionCell;
class AParticleRecord;
class PrimaryParticleTracker;
class S1_Generator;
class S2_Generator;
class AParticleGun;
class QProcess;

class AParticleSourceSimulator : public ASimulator
{
public:
    explicit AParticleSourceSimulator(const DetectorClass * detector, ASimulationManager * simMan, int ID);
    ~AParticleSourceSimulator();

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

    int totalEventCount = 0;
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

#endif // APARTICLESOURCESIMULATOR_H
