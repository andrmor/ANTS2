#include "aeventtrackingrecord.h"

AParticleTrackingRecord::AParticleTrackingRecord(int ParticleId, double StartEnergy, double * StartPosition, AParticleTrackingRecord * SecondaryOf) :
    ParticleId(ParticleId), StartEnergy(StartEnergy), SecondaryOf(SecondaryOf)
{
    for (int i=0; i<3; i++)
        this->StartPosition[i] = StartPosition[i];
}

AParticleTrackingRecord *AParticleTrackingRecord::create(int ParticleId, double StartEnergy, double * StartPosition, AParticleTrackingRecord * SecondaryOf)
{
    return new AParticleTrackingRecord(ParticleId, StartEnergy, StartPosition, SecondaryOf);
}

void AParticleTrackingRecord::addStep(ATrackingStepData * step)
{
    Steps.push_back( step );
}

// ============= Event ==============

AEventTrackingRecord * AEventTrackingRecord::create()
{
    return new AEventTrackingRecord();
}

AEventTrackingRecord::AEventTrackingRecord() {}

void AEventTrackingRecord::addParticleTrackingRecord(AParticleTrackingRecord *rec)
{
    PrimaryParticleRecords.push_back(rec);
}






