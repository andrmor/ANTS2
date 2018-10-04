#ifndef AOPTICALOVERRIDESCRIPTINTERFACE_H
#define AOPTICALOVERRIDESCRIPTINTERFACE_H

#include "ascriptinterface.h"
#include "aopticaloverride.h"

#include <QVariant>
#include <QScriptValue>

class APhoton;
class AMaterialParticleCollection;

class AOpticalOverrideScriptInterface : public AScriptInterface
{
    Q_OBJECT
public:
    AOpticalOverrideScriptInterface(const AMaterialParticleCollection* MpCollection, TRandom2* RandGen);

    void configure(APhoton *Photon, const double *NormalVector, int MatFrom, int MatTo);
    AOpticalOverride::OpticalOverrideResultEnum getResult(AOpticalOverride::ScatterStatusEnum& status);

public slots:

    //ToDo: meddling test! -> e.g. dir was changed and then Lambert called
    //maybe implement abort and add it after directives -> tested: x2 slow down

    void Absorb();
    void SpecularReflection();
    void Isotropic();
    void LambertBack();
    void LambertForward();
    void Fresnel();
    bool TryTransmissionSnell();
    void TransmissionDirect();

    QVariant GetNormal();
    double GetAngleWithNormal(bool inRadians);

    QVariant GetDirection();
    void SetDirection(double vx, double vy, double vz);

    double GetTime();
    void SetTime(double time);
    void AddTime(double dt);

    double getWaveIndex();
    void setWaveIndex(int waveIndex);

    double getRefractiveIndexFrom();
    double GetRefractiveIndexTo();

    QVariant GetPosition();

    void Console(const QString text);

private:
    const AMaterialParticleCollection * MPcollection;
    TRandom2 * RandGen;
    APhoton * Photon;
    const double * NormalVector;
    int iMatFrom;
    int iMatTo;

    bool bResultAlreadySet;
    AOpticalOverride::OpticalOverrideResultEnum ReturnResult; //{NotTriggered, Absorbed, Forward, Back, _Error_};
    AOpticalOverride::ScatterStatusEnum Status;

signals:
    void requestAbort(const QScriptValue &result = QScriptValue());
};

#endif // AOPTICALOVERRIDESCRIPTINTERFACE_H
