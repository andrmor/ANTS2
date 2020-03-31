#ifndef APARTICLESOURCESIMULATOR_H
#define APARTICLESOURCESIMULATOR_H

#include "asimulator.h"

#include <vector>

#include <QVector>

#include <QObject>
#include <QProcess>

class AEnergyDepositionCell;
class AParticleRecord;
class AParticleTracker;// PrimaryParticleTracker;
class S1_Generator;
class S2_Generator;
class AParticleGun;
class QProcess;
class AEventTrackingRecord;
class AExternalProcessHandler;
class QTextStream;
class QFile;

class AParticleSourceSimulator : public ASimulator
{
public:
    explicit AParticleSourceSimulator(ASimulationManager & simMan, int ID);
    ~AParticleSourceSimulator();

    const QVector<AEnergyDepositionCell*> &getEnergyVector() const { return EnergyVector; }  //obsolete
    void ClearEnergyVectorButKeepObjects() {EnergyVector.resize(0);} //to avoid clear of objects stored in the vector  //obsolete

    virtual int getEventCount() const override { return eventEnd - eventBegin; }
    virtual int getEventsDone() const override { return eventCurrent - eventBegin; }
    virtual int getTotalEventCount() const override { return totalEventCount; }
    virtual bool setup(QJsonObject & json) override;
    virtual bool finalizeConfig() override;
    virtual void updateGeoManager() override;

    virtual void simulate() override;

    virtual void appendToDataHub(EventsDataClass * dataHub) override;
    virtual void mergeData() override;

    void setOnlySavePrimaries() {bOnlySavePrimariesToFile = true;} // for G4ants mode // obsolete??? ***!!!

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
    bool runGeant4Handler();

    bool processG4DepositionData();
    bool readG4DepoEventFromTextFile();
    bool readG4DepoEventFromBinFile(bool expectNewEvent = false);
    void releaseInputResources();
    bool prepareWorkerG4File();

    //local objects
    AParticleTracker* ParticleTracker = nullptr;
    S1_Generator* S1generator = nullptr;
    S2_Generator* S2generator = nullptr;
    AParticleGun* ParticleGun = nullptr;
    QVector<AEnergyDepositionCell*> EnergyVector;
    QVector<AParticleRecord*> ParticleStack;

    //resources for ascii input
    QFile *       inTextFile = nullptr;
    QTextStream * inTextStream = nullptr;
    QString       G4DepoLine;
    //resources for binary input
    std::ifstream * inStream = nullptr;
    int           G4NextEventId = -1;

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
    AExternalProcessHandler * G4handler = nullptr;
    bool bOnlySavePrimariesToFile = false;
    bool bG4isRunning = false;
    QSet<QString> SeenNonRegisteredParticles;
    double DepoByNotRegistered;
    double DepoByRegistered;
    std::vector<AEventTrackingRecord *> TrackingHistory;

};

#endif // APARTICLESOURCESIMULATOR_H
