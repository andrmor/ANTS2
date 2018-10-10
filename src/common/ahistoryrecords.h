#ifndef AHISTORYRECORDS
#define AHISTORYRECORDS

#include <QList>

struct MaterialHistoryStructure
{
    MaterialHistoryStructure(int mat, double en, double dis) {MaterialId = mat; DepositedEnergy = en; Distance = dis;}
    MaterialHistoryStructure(){}
    int MaterialId;
    double DepositedEnergy;
    double Distance;
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
    float dx, dy, dz; //direction
    float initialEnergy;
    TerminationTypes Termination;

    EventHistoryStructure(int ParticleId, int index, int SecondaryOf, double* v, double energy)
      : ParticleId(ParticleId), index(index), SecondaryOf(SecondaryOf), dx(v[0]), dy(v[1]), dz(v[2]), initialEnergy(energy) { }
    EventHistoryStructure(){}
    bool isSecondary() const {return SecondaryOf > -1;}

    QList<MaterialHistoryStructure> Deposition;

    //utilities
    static QStringList getAllDefinedTerminationTypes() {return QStringList({"NotFinished", "Escaped", "AllEnergyDisspated", "Photoelectric",
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

