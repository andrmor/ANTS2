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
    AOpticalOverride::OpticalOverrideResultEnum getResult() const {return ReturnResult;}

public slots:
    void Absorb();

private:
    TRandom2 * RandGen;
    APhoton * Photon;
    const double * NormalVector;

    AOpticalOverride::OpticalOverrideResultEnum ReturnResult; //{NotTriggered, Absorbed, Forward, Back, _Error_};
};

#endif // AOPTICALOVERRIDESCRIPTINTERFACE_H
