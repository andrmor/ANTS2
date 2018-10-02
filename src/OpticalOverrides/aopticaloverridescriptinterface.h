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
    void Absorb();
    void Fresnel();  //ToDo: meddling test!
    //void TransmissionSnell();  //ToDo: meddling test!
    void TransmissionDirect();  //ToDo: meddling test!
    void SpecularReflection();  //ToDo: meddling test!
    void Isotropic();  //ToDo: meddling test!
    void LambertBack();  //ToDo: meddling test!
    void LambertForward();  //ToDo: meddling test!

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
