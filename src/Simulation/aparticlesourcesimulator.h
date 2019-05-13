#ifndef APARTICLESOURCESIMULATOR_H
#define APARTICLESOURCESIMULATOR_H

#include "asimulator.h"

#include <vector>

#include <QVector>

class AEnergyDepositionCell;
class AParticleRecord;
class AParticleTracker;// PrimaryParticleTracker;
class S1_Generator;
class S2_Generator;
class AParticleGun;
class QProcess;
class AEventTrackingRecord;

class AParticleSourceSimulator : public ASimulator
{
public:
    explicit AParticleSourceSimulator(const DetectorClass * detector, ASimulationManager * simMan, int ID);
    ~AParticleSourceSimulator();

    const QVector<AEnergyDepositionCell*> &getEnergyVector() const { return EnergyVector; }  //obsolete
    void ClearEnergyVectorButKeepObjects() {EnergyVector.resize(0);} //to avoid clear of objects stored in the vector  //obsolete

    virtual int getEventCount() const override { return eventEnd - eventBegin; }
    virtual int getEventsDone() const override { return eventCurrent - eventBegin; }
    virtual int getTotalEventCount() const override { return totalEventCount; }
    virtual bool setup(QJsonObject & json) override;
    virtual void updateGeoManager() override;
    virtual void simulate() override;
    virtual void appendToDataHub(EventsDataClass * dataHub) override;
    virtual void mergeData() override;

    void setOnlySavePrimaries() {bOnlySavePrimariesToFile = true;} // for G4ants mode

    virtual void hardAbort() override;

protected:
    virtual void updateMaxTracks(int maxPhotonTracks, int maxParticleTracks) override;

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
    //PrimaryParticleTracker* ParticleTracker = 0;
    AParticleTracker* ParticleTracker = 0;
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
    QSet<QString> SeenNonRegisteredParticles;
    double DepoByNotRegistered;
    double DepoByRegistered;
    std::vector<AEventTrackingRecord *> TrackingHistory;

};

#endif // APARTICLESOURCESIMULATOR_H
