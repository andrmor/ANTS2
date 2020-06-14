#ifndef APARTICLESOURCESIMULATOR_H
#define APARTICLESOURCESIMULATOR_H

#include "asimulator.h"

#include <vector>

#include <QVector>
#include <QObject>
#include <QProcess>

class AParticleSimSettings;
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
    explicit AParticleSourceSimulator(const ASimSettings & SimSet, const DetectorClass & detector, int threadIndex, int startSeed, AParticleSimSettings & partSimSet);
    ~AParticleSourceSimulator();

    int  getEventCount() const override {return eventEnd - eventBegin;}
    int  getEventsDone() const override;
    int  getTotalEventCount() const override {return totalEventCount;}
    bool setup() override;
    bool finalizeConfig() override;
    void updateGeoManager() override;

    void simulate() override;

    void appendToDataHub(EventsDataClass * dataHub) override;
    void hardAbort() override;

    void mergeData(QSet<QString> & SeenNonReg, double & DepoNotReg, double & DepoReg, std::vector<AEventTrackingRecord *> & TrHistory);

    const QVector<AEnergyDepositionCell*> & getEnergyVector() const { return EnergyVector; }  // !*! to change
    void ClearEnergyVectorButKeepObjects() {EnergyVector.resize(0);} //to avoid clear of objects stored in the vector  // !*! to change
    const AParticleGun * getParticleGun() const {return ParticleGun;}  // !*! to remove

protected:
    void updateMaxTracks(int maxPhotonTracks, int maxParticleTracks) override;

private:
    void EnergyVectorToScan();
    void clearParticleStack();
    void clearEnergyVector();
    void clearGeneratedParticles();

    int  chooseNumberOfParticlesThisEvent() const;
    bool choosePrimariesForThisEvent(int numPrimaries, int iEvent);
    bool generateAndTrackPhotons();
    bool geant4TrackAndProcess();
    bool runGeant4Handler();

    bool processG4DepositionData();
    bool readG4DepoEventFromTextFile();
    bool readG4DepoEventFromBinFile(bool expectNewEvent = false);
    void releaseInputResources();
    bool prepareWorkerG4File();
    void generateG4antsConfigCommon(QJsonObject & json);
    void removeOldFile(const QString &fileName, const QString &txt);

private:
    AParticleSimSettings & partSimSet;  // !*! TODO refactor, now cannot be const due to FileGenSettings
    const AParticleSimSettings & PartSimSet;

    //local objects
    AParticleTracker * ParticleTracker = nullptr;
    S1_Generator     * S1generator     = nullptr;
    S2_Generator     * S2generator     = nullptr;
    AParticleGun     * ParticleGun     = nullptr;

    QVector<AEnergyDepositionCell*> EnergyVector;
    QVector<AParticleRecord*>       ParticleStack;

    //resources for ascii input
    QFile         * inTextFile    = nullptr;
    QTextStream   * inTextStream  = nullptr;
    QString         G4DepoLine;
    //resources for binary input
    std::ifstream * inStream      = nullptr;
    int             G4NextEventId = -1;

    //local use - container which particle generator fills for each event; the particles are deleted by the tracker
    QVector<AParticleRecord*> GeneratedParticles;

    int totalEventCount = 0;
    double timeFrom, timeRange;   // !*! to remove
    double updateFactor;

    //Geant4 interface
    AExternalProcessHandler * G4handler = nullptr;
    bool bOnlySavePrimariesToFile = false;     // !*! obsolete?
    bool bG4isRunning = false;
    QSet<QString> SeenNonRegisteredParticles;
    double DepoByNotRegistered = 0;
    double DepoByRegistered = 0;
    std::vector<AEventTrackingRecord *> TrackingHistory;
    int StartSeed;
};

#endif // APARTICLESOURCESIMULATOR_H
