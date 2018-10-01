#include "aopticaloverridescriptinterface.h"

AOpticalOverrideScriptInterface::AOpticalOverrideScriptInterface()
{

}

void AOpticalOverrideScriptInterface::configure(TRandom2 *randGen, APhoton *photon, const double *normalVector)
{
    RandGen = randGen;
    Photon = photon;
    NormalVector = normalVector;

    ReturnResult = AOpticalOverride::NotTriggered;
}

void AOpticalOverrideScriptInterface::Absorb()
{
    ReturnResult = AOpticalOverride::Absorbed;
}
