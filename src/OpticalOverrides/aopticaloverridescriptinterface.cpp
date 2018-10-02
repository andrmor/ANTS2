#include "aopticaloverridescriptinterface.h"
#include "aphoton.h"

#include <QDebug>

AOpticalOverrideScriptInterface::AOpticalOverrideScriptInterface() {}

void AOpticalOverrideScriptInterface::configure(TRandom2 * randGen, APhoton *photon, const double *normalVector)
{
    RandGen = randGen;
    Photon = photon;
    NormalVector = normalVector;

    //if script did not modify any photon properties, assuming override is skipped (doing Fresnel):
    bResultAlreadySet = true;
    ReturnResult = AOpticalOverride::NotTriggered;
    Status = AOpticalOverride::Fresnel;
}

AOpticalOverride::OpticalOverrideResultEnum AOpticalOverrideScriptInterface::getResult(AOpticalOverride::ScatterStatusEnum& status)
{
    if (bResultAlreadySet)
    {
        status = Status;
        return ReturnResult;
    }

    Photon->EnsureUnitaryLength();
    double sum = 0;
    for (int i=0; i<3; i++)
        sum += Photon->v[i] * NormalVector[i];

        //qDebug() << "New Dir"<<Photon->v[0]<<Photon->v[1]<<Photon->v[2];
        //qDebug() << "sum"<<sum;

    if (sum < 0)
    {
        Status = AOpticalOverride::UnclassifiedReflection;
        return AOpticalOverride::Back;
    }
    else
    {
        Status = AOpticalOverride::Transmission;
        return AOpticalOverride::Forward;
    }
}

void AOpticalOverrideScriptInterface::Absorb()
{
    ReturnResult = AOpticalOverride::Absorbed;
    Status = AOpticalOverride::Absorption;
    bResultAlreadySet = true;
}

void AOpticalOverrideScriptInterface::Fresnel()
{
    ReturnResult = AOpticalOverride::NotTriggered;
    Status = AOpticalOverride::Fresnel;
    bResultAlreadySet = true;
}

void AOpticalOverrideScriptInterface::TransmissionDirect()
{
    ReturnResult = AOpticalOverride::Forward;
    Status = AOpticalOverride::Transmission;
    bResultAlreadySet = true;
}

void AOpticalOverrideScriptInterface::SpecularReflection()
{
    double NK = 0;
    for (int i=0; i<3; i++) NK += NormalVector[i] * Photon->v[i];
    for (int i=0; i<3; i++) Photon->v[i] -= 2.0*NK*NormalVector[i];

    //Photon->SimStat->OverrideSimplisticReflection++;
    Status = AOpticalOverride::SpikeReflection;
    ReturnResult = AOpticalOverride::Back;
    bResultAlreadySet = true;
}

void AOpticalOverrideScriptInterface::Isotropic()
{
    Photon->RandomDir(RandGen);
    bResultAlreadySet = false;
}

void AOpticalOverrideScriptInterface::SetDirection(double vx, double vy, double vz)
{
    Photon->v[0] = vx;
    Photon->v[1] = vy;
    Photon->v[2] = vz;
    bResultAlreadySet = false;
}
