#ifndef AOPTICALOVERRIDESCRIPTINTERFACE_H
#define AOPTICALOVERRIDESCRIPTINTERFACE_H

#include "ascriptinterface.h"
#include "aopticaloverride.h"

#include <QObject>

class TRandom2;
class APhoton;

class AOpticalOverrideScriptInterface : public AScriptInterface
{
    Q_OBJECT
public:
    AOpticalOverrideScriptInterface();

    void configure(TRandom2 *RandGen, APhoton *Photon, const double *NormalVector);
    AOpticalOverride::OpticalOverrideResultEnum getResult() const;

public slots:
    void Absorb();
    void Fresnel();
    //void TransmissionSnell();  //check was meddled
    void TransmissionDirect();  //check was meddled

    void SetDirection(double vx, double vy, double vz);

private:
    TRandom2 * RandGen;
    APhoton * Photon;
    const double * NormalVector;

    bool bResultAlreadySet;
    AOpticalOverride::OpticalOverrideResultEnum ReturnResult; //{NotTriggered, Absorbed, Forward, Back, _Error_};
};

#endif // AOPTICALOVERRIDESCRIPTINTERFACE_H
