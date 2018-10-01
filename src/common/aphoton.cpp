#include "aphoton.h"

#include "TRandom2.h"
#include "TMath.h"

APhoton::APhoton() {}

APhoton::APhoton(double *xyz, double *Vxyz, int waveIndex, double time) :
    time(time), waveIndex(waveIndex), scint_type(0)
{
    r[0]=xyz[0];  r[1]=xyz[1];  r[2]=xyz[2];
    v[0]=Vxyz[0]; v[1]=Vxyz[1]; v[2]=Vxyz[2];
}

void APhoton::CopyFrom(const APhoton *CopyFrom)
{
    r[0] = CopyFrom->r[0];
    r[1] = CopyFrom->r[1];
    r[2] = CopyFrom->r[2];

    v[0] = CopyFrom->v[0];
    v[1] = CopyFrom->v[1];
    v[2] = CopyFrom->v[2];

    time = CopyFrom->time;
    waveIndex = CopyFrom->waveIndex;
    scint_type = CopyFrom->scint_type;
    fSkipThisPhoton = CopyFrom->fSkipThisPhoton;

    SimStat = CopyFrom->SimStat;
}

void APhoton::EnsureUnitaryLength()
{
    double mod;
    for (int i=0; i<3; i++)
        mod += ( v[i] * v[i] );
    mod = TMath::Sqrt(mod);

    if (mod != 0)
        for (int i=0; i<3; i++) v[i] /= mod;
    else
    {
        v[0] = 0; v[1] = 0; v[2] = 1.0;
    }
}

void APhoton::RandomDir(TRandom2 *RandGen)
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
