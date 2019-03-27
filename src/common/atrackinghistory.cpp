#include "atrackinghistory.h"

ATrackingHistory::ATrackingHistory(int ParticleId, double StartEnergy, double * StartPosition, ATrackingHistory * SecondaryOf) :
    ParticleId(ParticleId), StartEnergy(StartEnergy), SecondaryOf(SecondaryOf)
{
    for (int i=0; i<3; i++)
        this->StartPosition[i] = StartPosition[i];
}

ATrackingHistory *ATrackingHistory::create(int ParticleId, double StartEnergy, double * StartPosition, ATrackingHistory * SecondaryOf)
{
    return new ATrackingHistory(ParticleId, StartEnergy, StartPosition, SecondaryOf);
}

void ATrackingHistory::addStep(ATrackingStepData * step)
{
    Steps.push_back( step );
}


