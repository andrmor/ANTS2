#ifndef S1_GENERATOR_H
#define S1_GENERATOR_H

#include "ahistoryrecords.h"

#include <QVector>

struct AEnergyDepositionCell;
class AMaterial;
class APhotonTracer;
class Photon_Generator;
class AMaterialParticleCollection;
class TRandom2;

class S1_Generator
{
public:
    explicit S1_Generator(Photon_Generator* photonGenerator,
                          APhotonTracer* photonTracker,
                          AMaterialParticleCollection* materialCollection,
                          QVector<AEnergyDepositionCell*>* energyVector,
                          QVector<GeneratedPhotonsHistoryStructure>* PhotonsHistory,
                          TRandom2* RandomGenerator);

    bool Generate();  //uses EnergyVector as the input parameter
    void setDoTextLog(bool flag){DoTextLog = flag;}

private:
    QVector<AEnergyDepositionCell*>* EnergyVector = nullptr;
    Photon_Generator* PhotonGenerator = nullptr;
    APhotonTracer* PhotonTracker = nullptr;
    TRandom2* RandGen = nullptr;
    AMaterialParticleCollection* MaterialCollection = nullptr;
    QVector<GeneratedPhotonsHistoryStructure>* GeneratedPhotonsHistory = nullptr;

    bool DoTextLog = false;
};

#endif // S1_GENERATOR_H
