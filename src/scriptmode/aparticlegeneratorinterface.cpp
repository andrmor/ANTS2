#include "aparticlegeneratorinterface.h"
#include "amaterialparticlecolection.h"
#include "aparticlerecord.h"

#include "TMath.h"
#include "TRandom2.h"

AParticleGeneratorInterface::AParticleGeneratorInterface(const AMaterialParticleCollection & MpCollection, TRandom2 *RandGen) :
    MpCollection(MpCollection), RandGen(RandGen) {}

void AParticleGeneratorInterface::configure(QVector<AParticleRecord*> * GeneratedParticles)
{
    GP = GeneratedParticles;
}

void AParticleGeneratorInterface::AddParticle(int type, double energy, double x, double y, double z, double i, double k, double j, double time)
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

void AParticleGeneratorInterface::AddParticleIsotropic(int type, double energy, double x, double y, double z, double time)
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
