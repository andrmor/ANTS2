#include "aeventtrackingrecord.h"
#include "atrackrecords.h"
#include "atrackbuildoptions.h"

#include <QStringList>
#include <QDebug>

// ============= Step ==============

ATrackingStepData::ATrackingStepData(float *position, float time, float depositedEnergy, const QString & process) :
    Time(time), DepositedEnergy(depositedEnergy), Process(process)
{
    for (int i=0; i<3; i++) Position[i] = position[i];
}

ATrackingStepData::ATrackingStepData(float x, float y, float z, float time, float depositedEnergy, const QString &process) :
    Time(time), DepositedEnergy(depositedEnergy), Process(process)
{
    Position[0] = x;
    Position[1] = y;
    Position[2] = z;
}

void ATrackingStepData::addSecondary(AParticleTrackingRecord *sec)
{
    Secondaries.push_back(sec);
}

int ATrackingStepData::countSecondaries() const
{
    return static_cast<int>(Secondaries.size());
}

void ATrackingStepData::logToString(QString & str, int offset) const
{
    str += QString(' ').repeated(offset);
    str += QString("%1 dE=%2 at (%3, %4, %5) %6").arg(Process).arg(DepositedEnergy).arg(Position[0]).arg(Position[1]).arg(Position[2]).arg(Time);
    if (Secondaries.size() > 0)
        str += QString("  #sec:%1").arg(Secondaries.size());
    str += '\n';
}

// ============= Track ==============

AParticleTrackingRecord::AParticleTrackingRecord(const QString & particle, float startEnergy, float * startPosition, float time) :
    ParticleName(particle), StartEnergy(startEnergy), StartTime(time)
{
    for (int i=0; i<3; i++)
        StartPosition[i] = startPosition[i];
}

AParticleTrackingRecord::AParticleTrackingRecord(const QString & particle,  float startEnergy, float startX, float startY, float startZ, float time) :
    ParticleName(particle), StartEnergy(startEnergy), StartTime(time)
{
    StartPosition[0] = startX;
    StartPosition[1] = startY;
    StartPosition[2] = startZ;
}

AParticleTrackingRecord::~AParticleTrackingRecord()
{
    for (auto* step : Steps) delete step;
    for (auto* sec  : Secondaries) delete sec;
}

AParticleTrackingRecord *AParticleTrackingRecord::create(const QString & Particle, double StartEnergy, double * StartPosition, double Time)
{
    float pos[3];
    for (int i=0; i<3; i++) pos[i] = static_cast<float>(StartPosition[i]);

    return new AParticleTrackingRecord(Particle,
                                       static_cast<float>(StartEnergy),
                                       pos,
                                       static_cast<float>(Time));
}

AParticleTrackingRecord *AParticleTrackingRecord::create(const QString & Particle, float StartEnergy, float StartX, float StartY, float StartZ, float Time)
{
    return new AParticleTrackingRecord(Particle, StartEnergy, StartX, StartY, StartZ, Time);
}

AParticleTrackingRecord *AParticleTrackingRecord::create()
{
    return new AParticleTrackingRecord("undefined", 0, 0, 0, 0, 0);
}

void AParticleTrackingRecord::update(const QString & particle, float startEnergy, float startX, float startY, float startZ, float time)
{
    ParticleName     = particle;
    StartEnergy      = startEnergy;
    StartPosition[0] = startX;
    StartPosition[1] = startY;
    StartPosition[2] = startZ;
    StartTime        = time;
}

AParticleTrackingRecord *AParticleTrackingRecord::create(const QString & Particle, float StartEnergy, float *StartPosition, float Time)
{
    return new AParticleTrackingRecord(Particle, StartEnergy, StartPosition, Time);
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

void AParticleTrackingRecord::logToString(QString & str, int offset, const QStringList & ParticleNames, bool bExpandSecondaries) const
{
    str += QString(' ').repeated(offset) + '>';
    //str += (ParticleId > -1 && ParticleId < ParticleNames.size() ? ParticleNames.at(ParticleId) : "unknown");
    str += ParticleName;
    str += QString("  E=%1  at (%2, %3, %4) %5\n").arg(StartEnergy).arg(StartPosition[0]).arg(StartPosition[1]).arg(StartPosition[2]).arg(StartTime);

    for (auto* st : Steps)
    {
        st->logToString(str, offset);
        if (bExpandSecondaries)
        {
            const std::vector<AParticleTrackingRecord *> & StepSecondaries = st->getSecondaries();
            for (auto * sec : StepSecondaries)
                sec->logToString(str, offset + 4, ParticleNames, bExpandSecondaries);
        }
    }
}

void AParticleTrackingRecord::makeTrack(std::vector<TrackHolderClass *> & Tracks, const QStringList & ParticleNames, const ATrackBuildOptions & TrackBuildOptions, bool bWithSecondaries) const
{
    TrackHolderClass * tr = new TrackHolderClass();
    tr->Nodes.append( TrackNodeStruct(StartPosition[0], StartPosition[1], StartPosition[2], StartTime) );
    tr->UserIndex = 22;
    TrackBuildOptions.applyToParticleTrack(tr, ParticleNames.indexOf(ParticleName));
    Tracks.push_back(tr);

    for (ATrackingStepData * step : Steps)
        tr->Nodes.append( TrackNodeStruct(step->Position[0], step->Position[1], step->Position[2], step->Time) );

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
