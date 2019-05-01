#include "aeventtrackingrecord.h"
#include "atrackrecords.h"
#include "atrackbuildoptions.h"

#include <QDebug>

#include "TGeoManager.h"
#include "TGeoNode.h"

// ============= Step ==============

ATrackingStepData::ATrackingStepData(float *position, float time, float energy, float depositedEnergy, const QString & process) :
    Time(time), Energy(energy), DepositedEnergy(depositedEnergy), Process(process)
{
    for (int i=0; i<3; i++) Position[i] = position[i];
}

ATrackingStepData::ATrackingStepData(float x, float y, float z, float time, float energy, float depositedEnergy, const QString &process) :
    Time(time), Energy(energy), DepositedEnergy(depositedEnergy), Process(process)
{
    Position[0] = x;
    Position[1] = y;
    Position[2] = z;
}

void ATrackingStepData::logToString(QString & str, int offset) const
{
    str += QString(' ').repeated(offset);
    //str += QString("E=%1keV %2 depo=%3keV at [%4, %5, %6] t=%7ns").arg(Energy).arg(Process).arg(DepositedEnergy).arg(Position[0]).arg(Position[1]).arg(Position[2]).arg(Time);
    str += QString("At [%4, %5, %6]mm t=%7ns   %2   depo=%3keV E=%1keV").arg(Energy).arg(Process).arg(DepositedEnergy).arg(Position[0]).arg(Position[1]).arg(Position[2]).arg(Time);
    if (Secondaries.size() > 0)
        str += QString("  #sec:%1").arg(Secondaries.size());
    str += '\n';
}

// ============= Track ==============

AParticleTrackingRecord::~AParticleTrackingRecord()
{
    for (auto* step : Steps) delete step;
    for (auto* sec  : Secondaries) delete sec;
}

AParticleTrackingRecord *AParticleTrackingRecord::create(const QString & Particle)
{
    return new AParticleTrackingRecord(Particle);
}

AParticleTrackingRecord *AParticleTrackingRecord::create()
{
    return new AParticleTrackingRecord("undefined");
}

void AParticleTrackingRecord::updatePromisedSecondary(const QString & particle, float startEnergy, float startX, float startY, float startZ, float startTime)
{
    ParticleName = particle;
    Steps.push_back(new ATrackingStepData(startX, startY, startZ, startTime, startEnergy, 0, "C"));
}

void AParticleTrackingRecord::addStep(ATrackingStepData * step)
{
    Steps.push_back( step );
}

void AParticleTrackingRecord::addSecondary(AParticleTrackingRecord *sec)
{
    sec->SecondaryOf = this;
    Secondaries.push_back(sec);
}

int AParticleTrackingRecord::countSecondaries() const
{
    return static_cast<int>(Secondaries.size());
}

void AParticleTrackingRecord::logToString(QString & str, int offset, bool bExpandSecondaries) const
{
    str += QString(' ').repeated(offset) + '>';
    //str += (ParticleId > -1 && ParticleId < ParticleNames.size() ? ParticleNames.at(ParticleId) : "unknown");
    str += ParticleName + "\n";

    for (auto* st : Steps)
    {
        st->logToString(str, offset);
        if (bExpandSecondaries)
        {
            for (int & iSec : st->Secondaries)
                Secondaries.at(iSec)->logToString(str, offset + 4, bExpandSecondaries);
        }
    }
}

void AParticleTrackingRecord::makeTrack(std::vector<TrackHolderClass *> & Tracks, const QStringList & ParticleNames, const ATrackBuildOptions & TrackBuildOptions, bool bWithSecondaries) const
{
    TrackHolderClass * tr = new TrackHolderClass();
    tr->UserIndex = 22;
    TrackBuildOptions.applyToParticleTrack(tr, ParticleNames.indexOf(ParticleName));
    Tracks.push_back(tr);

    for (ATrackingStepData * step : Steps)
    {
        if (step->Process != "T")
            tr->Nodes.append( TrackNodeStruct(step->Position[0], step->Position[1], step->Position[2], step->Time) );
    }

    if (bWithSecondaries)
        for (AParticleTrackingRecord * sec : Secondaries)
            sec->makeTrack(Tracks, ParticleNames, TrackBuildOptions, bWithSecondaries);
}

// ============= Event ==============

AEventTrackingRecord * AEventTrackingRecord::create()
{
    return new AEventTrackingRecord();
}

AEventTrackingRecord::AEventTrackingRecord() {}

AEventTrackingRecord::~AEventTrackingRecord()
{
    for (auto* pr : PrimaryParticleRecords) delete pr;
}

void AEventTrackingRecord::addPrimaryRecord(AParticleTrackingRecord *rec)
{
    PrimaryParticleRecords.push_back(rec);
}

int AEventTrackingRecord::countPrimaries() const { return static_cast<int>(PrimaryParticleRecords.size()); }
