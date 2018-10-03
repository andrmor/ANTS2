#ifndef AOPTICALOVERRIDESCRIPTINTERFACE_H
#define AOPTICALOVERRIDESCRIPTINTERFACE_H

#include "ascriptinterface.h"
#include "aopticaloverride.h"

#include <QVariant>

class APhoton;

class AOpticalOverrideScriptInterface : public AScriptInterface
{
    Q_OBJECT
public:
    AOpticalOverrideScriptInterface();

    void configure(TRandom2 * RandGen, APhoton *Photon, const double *NormalVector);
    AOpticalOverride::OpticalOverrideResultEnum getResult(AOpticalOverride::ScatterStatusEnum& status);

public slots:

    //ToDo: meddling test! -> e.g. dir was changed and then Lambert called

    void Absorb();
    void SpecularReflection();
    void Isotropic();
    void LambertBack();
    void LambertForward();
    void Fresnel();
    //void TransmissionSnell();
    void TransmissionDirect();

    QVariant GetNormal();

    QVariant GetDirection();
    void SetDirection(double vx, double vy, double vz);

    double GetTime();
    void SetTime(double time);
    void AddTime(double dt);

    void Report(const QString text);

private:
    TRandom2 * RandGen;
    APhoton * Photon;
    const double * NormalVector;

    bool bResultAlreadySet;
    AOpticalOverride::OpticalOverrideResultEnum ReturnResult; //{NotTriggered, Absorbed, Forward, Back, _Error_};
    AOpticalOverride::ScatterStatusEnum Status;
};

#endif // AOPTICALOVERRIDESCRIPTINTERFACE_H
