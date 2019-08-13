#ifndef S2_GENERATOR_H
#define S2_GENERATOR_H

#include "ahistoryrecords.h"

#include <QObject>

struct AEnergyDepositionCell;
class Photon_Generator;
class TRandom2;
class TGeoManager;
class AMaterial;
class AMaterialParticleCollection;
class APhotonTracer;

class S2_Generator : public QObject
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
    QVector<AEnergyDepositionCell*>* EnergyVector;
    Photon_Generator* PhotonGenerator;
    APhotonTracer* PhotonTracker;
    TRandom2* RandGen;
    TGeoManager* GeoManager;
    AMaterialParticleCollection* MaterialCollection;
    QVector<GeneratedPhotonsHistoryStructure>* GeneratedPhotonsHistory;

    bool DoTextLog;
    bool OnlySecondary;
};

#endif // S2_GENERATOR_H
