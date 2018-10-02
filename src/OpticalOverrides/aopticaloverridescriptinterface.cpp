#include "aopticaloverridescriptinterface.h"
#include "aphoton.h"

#include <QDebug>
#include <QVariantList>

#include "TMath.h"

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

void AOpticalOverrideScriptInterface::LambertBack()
{
    double norm2;
    do
    {
        Photon->RandomDir(RandGen);
        for (int i=0; i<3; i++) Photon->v[i] -= NormalVector[i];
        norm2 = 0;
        for (int i=0; i<3; i++) norm2 += Photon->v[i] * Photon->v[i];
    }
    while (norm2 < 0.000001);

    double normInverted = 1.0/TMath::Sqrt(norm2);
    for (int i=0; i<3; i++) Photon->v[i] *= normInverted;
    Status = AOpticalOverride::LambertianReflection;
    ReturnResult = AOpticalOverride::Back;
    bResultAlreadySet = true;
}

void AOpticalOverrideScriptInterface::LambertForward()
{
    double norm2;
    do
    {
        Photon->RandomDir(RandGen);
        for (int i=0; i<3; i++) Photon->v[i] += NormalVector[i];
        norm2 = 0;
        for (int i=0; i<3; i++) norm2 += Photon->v[i] * Photon->v[i];
    }
    while (norm2 < 0.000001);

    double normInverted = 1.0/TMath::Sqrt(norm2);
    for (int i=0; i<3; i++) Photon->v[i] *= normInverted;
    Status = AOpticalOverride::Transmission;
    ReturnResult = AOpticalOverride::Forward;
    bResultAlreadySet = true;
}

QVariant AOpticalOverrideScriptInterface::GetDirection()
{
    QVariantList vl;
    vl << Photon->v[0] << Photon->v[1] << Photon->v[2];
    return vl;
}

void AOpticalOverrideScriptInterface::SetDirection(double vx, double vy, double vz)
{
    Photon->v[0] = vx;
    Photon->v[1] = vy;
    Photon->v[2] = vz;
    bResultAlreadySet = false;
}

void AOpticalOverrideScriptInterface::SetTime(double time)
{
    Photon->time = time;
}

void AOpticalOverrideScriptInterface::AddTime(double dt)
{
    Photon->time += dt;
}

void AOpticalOverrideScriptInterface::Report(const QString text)
{
    qDebug() << text;
}

double AOpticalOverrideScriptInterface::GetTime()
{
    return Photon->time;
}
