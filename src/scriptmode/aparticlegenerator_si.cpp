#include "aparticlegenerator_si.h"
#include "amaterialparticlecolection.h"
#include "aparticlerecord.h"

#include "TMath.h"
#include "TRandom2.h"

AParticleGenerator_SI::AParticleGenerator_SI(const AMaterialParticleCollection & MpCollection, TRandom2 *RandGen, int ThreadId) :
    MpCollection(MpCollection), RandGen(RandGen), ThreadId(ThreadId)
{
    H["AddParticle"] = "Adds particle to track to this event";
    H["AddParticleIsotropic"] = "Adds particle to track to this event. The particle's direction is randomly generated (isotropic)";

    H["StoreVariables"] = "Store array of variables: allows to have 'static' variables over the whole simulation";
    H["StoreVariables"] = "Retrive array of variables: allows to have 'static' variables over the whole simulation\nOn simulation start retrive will produce an empty array";
    H["GetThreadId"] = "Returns thread index: it is 0 in single thread simulation, and will be thread index (0 .. N) when simulation is perfromed in N threads.\nWarning: starting a simulation of 3 events when max threads is set to 7 will still have thread indexes only from 1 to 3!";
}

void AParticleGenerator_SI::configure(QVector<AParticleRecord*> * GeneratedParticles)
{
    GP = GeneratedParticles;
}

void AParticleGenerator_SI::AddParticle(int type, double energy, double x, double y, double z, double i, double k, double j, double time)
{
    if (type < 0 || type >= MpCollection.countParticles())
        abort("Invalid particle Id");
    else
    {
        AParticleRecord * p = new AParticleRecord(type,
                                                  x, y, z,
                                                  i, k, j,
                                                  time, energy);
        p->ensureUnitaryLength();
        GP->append(p);
    }
}

void AParticleGenerator_SI::AddParticleIsotropic(int type, double energy, double x, double y, double z, double time)
{
    if (type < 0 || type >= MpCollection.countParticles())
        abort("Invalid particle Id");
    else
    {
        AParticleRecord * p = new AParticleRecord(type,
                                                  x, y, z,
                                                  0, 0, 1.0,
                                                  time, energy);
        p->randomDir(RandGen); //will generate unitary length
        GP->append(p);
    }
}

void AParticleGenerator_SI::StoreVariables(QVariantList array)
{
    StoredData = array;
}

QVariantList AParticleGenerator_SI::RetrieveVariables()
{
    return StoredData;
}
