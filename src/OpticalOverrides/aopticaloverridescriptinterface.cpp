#include "aopticaloverridescriptinterface.h"

#include "aphoton.h"

AOpticalOverrideScriptInterface::AOpticalOverrideScriptInterface()
{

}

void AOpticalOverrideScriptInterface::configure(TRandom2 *randGen, APhoton *photon, const double *normalVector)
{
    RandGen = randGen;
    Photon = photon;
    NormalVector = normalVector;

    //if script did not modify any photon properties, this will be the result:
    bResultAlreadySet = true;
    ReturnResult = AOpticalOverride::NotTriggered;
}

AOpticalOverride::OpticalOverrideResultEnum AOpticalOverrideScriptInterface::getResult() const
{
    if (bResultAlreadySet) return ReturnResult;

    Photon->ensureUnitaryLength();
    double sum = 0;
    for (int i=0; i<3; i++)
        sum += Photon->v[i] * NormalVector[i];
    if (sum < 0)
        return AOpticalOverride::Back;
    else return AOpticalOverride::Forward;
}

void AOpticalOverrideScriptInterface::Absorb()
{
    ReturnResult = AOpticalOverride::Absorbed;
    bResultAlreadySet = true;
}

void AOpticalOverrideScriptInterface::Fresnel()
{
    bResultAlreadySet = true;
    ReturnResult = AOpticalOverride::NotTriggered;
}

void AOpticalOverrideScriptInterface::TransmissionDirect()
{
    bResultAlreadySet = true;
    ReturnResult = AOpticalOverride::Forward;
}

#include <QDebug>
void AOpticalOverrideScriptInterface::SetDirection(double vx, double vy, double vz)
{
    Photon->v[0] = vx;
    Photon->v[1] = vy;
    Photon->v[2] = vz;
    bResultAlreadySet = false;
}
