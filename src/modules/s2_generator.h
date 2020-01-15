#ifndef S2_GENERATOR_H
#define S2_GENERATOR_H

#include "ahistoryrecords.h"

#include <QObject>
#include <QVector>

struct AEnergyDepositionCell;
class Photon_Generator;
class TRandom2;
class TGeoManager;
class AMaterial;
class AMaterialParticleCollection;
class APhotonTracer;
class TString;

struct DiffSigmas
{
    double sigmaTime; //ns
    double sigmaX;    //mm

    DiffSigmas(double sigmaTime, double sigmaX) : sigmaTime(sigmaTime), sigmaX(sigmaX) {}
    DiffSigmas() {}
};

class S2_Generator : public QObject  // ***!!! need QObject?
{
    Q_OBJECT

public:
    explicit S2_Generator(Photon_Generator* photonGenerator,
                          APhotonTracer* photonTracker,
                          QVector<AEnergyDepositionCell*>* energyVector,
                          TRandom2 *RandomGenerator,
                          TGeoManager* geoManager,
                          AMaterialParticleCollection* materialCollection,
                          QVector<GeneratedPhotonsHistoryStructure>* PhotonsHistory,
                          QObject *parent = 0);

    void UpdateGeoManager(TGeoManager* NewGeoManager) {GeoManager = NewGeoManager;} // *** obsolete?
    
    bool Generate(); //uses EnergyVector as the input parameter

    void setDoTextLog(bool flag){DoTextLog = flag;}
    void setOnlySecondary(bool flag){OnlySecondary = flag;} //determines how photon log is filled

signals:
    
public slots:

private:
    QVector<AEnergyDepositionCell*> * EnergyVector = nullptr;
    Photon_Generator * PhotonGenerator = nullptr;
    APhotonTracer * PhotonTracker = nullptr;
    TRandom2 * RandGen = nullptr;
    TGeoManager * GeoManager = nullptr;
    AMaterialParticleCollection * MaterialCollection = nullptr;
    QVector<GeneratedPhotonsHistoryStructure> * GeneratedPhotonsHistory = nullptr;

    bool DoTextLog;
    bool OnlySecondary;

    int RecordNumber = 0; //tracer for photon log index, not used if OnlySecondary is true

    int ThisId;
    int ThisIndex;
    int ThisEvent;
    int MatId;

    double DepositedEnergy;

    int NumElectrons;
    int NumPhotons;

    double PhotonRemainer;

    int    TextLogPhotons;
    double TextLogEnergy;

    int LastEvent;
    int LastIndex;
    int LastParticle;
    int LastMaterial;

    double BaseTime;
    QVector<DiffSigmas> DiffusionRecords;

private:
    void doDrift(TString & VolName);
    void generateLight(double * DepoPosition);
    void generateAndTracePhotons(double * Position, double Time, int NumPhotonsToGenerate, int MatIndexSecScint, double Zstart, double Zspan);

    bool initLogger();
    void updateLogger();
    void storeLog();

};

#endif // S2_GENERATOR_H
