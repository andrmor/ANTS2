#include "aparticlerecord.h"

#include "TMath.h"
#include "TRandom2.h"

AParticleRecord::AParticleRecord(int Id,
                                 double x, double y, double z,
                                 double vx, double vy, double vz,
                                 double time, double energy,
                                 int secondaryOf) :
    Id(Id), time(time), energy(energy), secondaryOf(secondaryOf)
{
    r[0] = x;
    r[1] = y;
    r[2] = z;

    v[0] = vx;
    v[1] = vy;
    v[2] = vz;
}

AParticleRecord *AParticleRecord::clone()
{
    return new AParticleRecord(Id, r[0], r[1], r[2], v[0], v[1], v[2], time, energy, secondaryOf);
}

void AParticleRecord::ensureUnitaryLength()
{
    double mod = 0;
    for (int i=0; i<3; i++)
        mod += ( v[i] * v[i] );

    if (mod == 1.0) return;
    mod = TMath::Sqrt(mod);

    if (mod != 0)
    {
        for (int i=0; i<3; i++)
            v[i] /= mod;
    }
    else
    {
        v[0] = 0;
        v[1] = 0;
        v[2] = 1.0;
    }
}

void AParticleRecord::randomDir(TRandom2 *RandGen)
{
    //Sphere function of Root:
    double a = 0, b = 0, r2 = 1.0;
    while (r2 > 0.25)
      {
        a  = RandGen->Rndm() - 0.5;
        b  = RandGen->Rndm() - 0.5;
        r2 =  a*a + b*b;
      }
    v[2] = ( -1.0 + 8.0 * r2 );
    double scale = 8.0 * TMath::Sqrt(0.25 - r2);
    v[0] = a*scale;
    v[1] = b*scale;
}
