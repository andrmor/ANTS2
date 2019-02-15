#ifndef AHISTORYRECORDS
#define AHISTORYRECORDS

#include "aparticlerecord.h"

#include <QVector>
#include <QList>

struct MaterialHistoryStructure
{
    MaterialHistoryStructure(int mat, double en, double dis) {MaterialId = mat; DepositedEnergy = en; Distance = dis;}
    MaterialHistoryStructure(){}
    int MaterialId;
    float DepositedEnergy;
    float Distance;
};

struct EventHistoryStructure
{
    enum TerminationTypes{NotFinished = 0,
                          Escaped = 1,
                          AllEnergyDisspated = 2,
                          Photoelectric = 3,
                          ComptonScattering = 4,
                          NeutronAbsorption = 5,
                          ErrorDuringTracking = 6,
                          CreatedOutside = 7,   // created outside defined geometry
                          FoundUntrackableMaterial = 8,
                          PairProduction = 9,
                          ElasticScattering = 10,
                          StoppedOnMonitor = 11
                         };  // keep the numbers - scripts operate with int

    int ParticleId;
    int index; // this is particle index! - "serial number" of the particle
    int SecondaryOf;
    float x, y, z;  //creation position
    float dx, dy, dz; //direction
    float initialEnergy; //energy when created
    TerminationTypes Termination;
    QVector<MaterialHistoryStructure> Deposition;

    EventHistoryStructure(int ParticleId, int index, int SecondaryOf, double* r, double* v, double energy) :
        ParticleId(ParticleId), index(index), SecondaryOf(SecondaryOf),
         x(r[0]),  y(r[1]),  z(r[2]),
        dx(v[0]), dy(v[1]), dz(v[2]), initialEnergy(energy) {}

    EventHistoryStructure(const AParticleRecord* p, int index) :
        ParticleId(p->Id), index(index), SecondaryOf(p->secondaryOf),
         x(p->r[0]),  y(p->r[1]),  z(p->r[2]),
        dx(p->v[0]), dy(p->v[1]), dz(p->v[2]),
        initialEnergy(p->energy) {}

    EventHistoryStructure(){}

    bool isSecondary() const {return SecondaryOf > -1;}

    //utilities
    static QStringList getAllDefinedTerminationTypes() {return QStringList({"NotFinished", "Escaped", "AllEnergyDissipated", "Photoelectric",
                                                                            "ComptonScattering", "NeutronAbsorption", "ErrorDuringTracking",
                                                                            "CreatedOutside", "FoundUntrackableMaterial", "PairProduction",
                                                                            "ElasticScattering", "StoppedOnMonitor"});}
};

struct GeneratedPhotonsHistoryStructure
{
    int event;
    int index; // to distinguish between particles with the same ParticleId (e.g. during the same event)
    int ParticleId;
    int MaterialId;
    double Energy;
    int PrimaryPhotons;
    int SecondaryPhotons;
};

#endif // AHISTORYRECORDS

