#include "aopticaloverridescriptinterface.h"
#include "aphoton.h"
#include "amaterialparticlecolection.h"
#include "asimulationstatistics.h"

#include <QDebug>
#include <QVariantList>

#include "TMath.h"
#include "TVector3.h"

AOpticalOverrideScriptInterface::AOpticalOverrideScriptInterface(const AMaterialParticleCollection *MpCollection, TRandom2 *RandGen) :
    AScriptInterface(), MPcollection(MpCollection), RandGen(RandGen) {}

void AOpticalOverrideScriptInterface::configure(APhoton *photon, const double *normalVector, int MatFrom, int MatTo)
{
    Photon = photon;
    NormalVector = normalVector;
    iMatFrom = MatFrom;
    iMatTo = MatTo;

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

bool AOpticalOverrideScriptInterface::TryTransmissionSnell()
{
    double NK = 0;
    for (int i=0; i<3; i++) NK += NormalVector[i] * Photon->v[i];
    double nFrom = (*MPcollection)[iMatFrom]->getRefractiveIndex(Photon->waveIndex);
    double nTo   = (*MPcollection)[iMatTo]  ->getRefractiveIndex(Photon->waveIndex);
    double nn = nFrom / nTo;
    double UnderRoot = 1.0 - nn*nn*(1.0 - NK*NK);
    if (UnderRoot < 0)
    {
        //conditions for total internal reflection
        return false;
    }
    const double tmp = nn * NK - TMath::Sqrt(UnderRoot);
    for (int i=0; i<3; i++) Photon->v[i] = -tmp * NormalVector[i] + nn * Photon->v[i];

    ReturnResult = AOpticalOverride::Forward;
    Status = AOpticalOverride::Transmission;
    bResultAlreadySet = true;

    return true;
}

void AOpticalOverrideScriptInterface::TransmissionDirect()
{
    ReturnResult = AOpticalOverride::Forward;
    Status = AOpticalOverride::Transmission;
    bResultAlreadySet = true;
}

QVariant AOpticalOverrideScriptInterface::GetNormal()
{
    QVariantList vl;
    vl << NormalVector[0] << NormalVector[1] << NormalVector[2];
    return vl;
}

double AOpticalOverrideScriptInterface::GetAngleWithNormal(bool inRadians)
{
    TVector3 N(NormalVector);
    TVector3 P(Photon->v);
    double angle = N.Angle(P);
    if (inRadians) return angle;
    else return 180.0/TMath::Pi()*angle;
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
    //emit requestAbort();
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
    Photon->SimStat->timeChanged++;
}

void AOpticalOverrideScriptInterface::AddTime(double dt)
{
    Photon->time += dt;
    Photon->SimStat->timeChanged++;
}

double AOpticalOverrideScriptInterface::getWaveIndex()
{
    return Photon->waveIndex;
}

void AOpticalOverrideScriptInterface::setWaveIndex(int waveIndex)
{
    Photon->waveIndex = waveIndex;
    Photon->SimStat->wavelengthChanged++;
}

double AOpticalOverrideScriptInterface::getRefractiveIndexFrom()
{
    return (*MPcollection)[iMatFrom]->getRefractiveIndex(Photon->waveIndex);
}

double AOpticalOverrideScriptInterface::GetRefractiveIndexTo()
{
    return (*MPcollection)[iMatTo]->getRefractiveIndex(Photon->waveIndex);
}

QVariant AOpticalOverrideScriptInterface::GetPosition()
{
    QVariantList vl;
    vl << Photon->r[0] << Photon->r[1] << Photon->r[2];
    return vl;
}

void AOpticalOverrideScriptInterface::Console(const QString text)
{
    qDebug() << text;
}

double AOpticalOverrideScriptInterface::GetTime()
{
    return Photon->time;
}
