#ifndef S1_GENERATOR_H
#define S1_GENERATOR_H

#include "ahistoryrecords.h"

#include <QObject>

struct AEnergyDepositionCell;
class AMaterial;
class APhotonTracer;
class Photon_Generator;
class AMaterialParticleCollection;
class TRandom2;

class S1_Generator : public QObject
{
    Q_OBJECT

public:
    explicit S1_Generator(Photon_Generator* photonGenerator,
                          APhotonTracer* photonTracker,
                          AMaterialParticleCollection* materialCollection,
                          QVector<AEnergyDepositionCell*>* energyVector,
                          QVector<GeneratedPhotonsHistoryStructure>* PhotonsHistory,
                          TRandom2* RandomGenerator,
                          QObject *parent = 0);

    bool Generate();  //uses EnergyVector as the input parameter
    void setDoTextLog(bool flag){DoTextLog = flag;}

private:
    QVector<AEnergyDepositionCell*>* EnergyVector;
    Photon_Generator* PhotonGenerator;
    APhotonTracer* PhotonTracker;
    TRandom2* RandGen;

    AMaterialParticleCollection* MaterialCollection;
    QVector<GeneratedPhotonsHistoryStructure>* GeneratedPhotonsHistory;

    bool DoTextLog;    
};

#endif // S1_GENERATOR_H
