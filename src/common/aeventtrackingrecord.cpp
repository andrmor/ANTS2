#include "aeventtrackingrecord.h"

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

const QString ATrackingStepData::toString(int offset) const
{
    QString s = QString(' ').repeated(offset) + QString("Pos=(%1,%2,%3)  Time=%4  dE=%5  %6").arg(Position[0]).arg(Position[1]).arg(Position[2]).arg(Time).arg(DepositedEnergy).arg(Process);
    if (Secondaries.size() > 0) s += QString("  #Secondaries:%1").arg(Secondaries.size());
    s += '\n';
    return s;
}

// ============= Track ==============

AParticleTrackingRecord::AParticleTrackingRecord(int particleId, float startEnergy, float * startPosition, float time) :
    ParticleId(particleId), StartEnergy(startEnergy), StartTime(time)
{
    for (int i=0; i<3; i++)
        StartPosition[i] = startPosition[i];
}

AParticleTrackingRecord::AParticleTrackingRecord(int particleId,  float startEnergy, float startX, float startY, float startZ, float time) :
    ParticleId(particleId), StartEnergy(startEnergy), StartTime(time)
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

AParticleTrackingRecord *AParticleTrackingRecord::create(int ParticleId, double StartEnergy, double * StartPosition, double Time)
{
    float pos[3];
    for (int i=0; i<3; i++) pos[i] = static_cast<float>(StartPosition[i]);

    return new AParticleTrackingRecord(ParticleId,
                                       static_cast<float>(StartEnergy),
                                       pos,
                                       static_cast<float>(Time));
}

AParticleTrackingRecord *AParticleTrackingRecord::create(int ParticleId, float StartEnergy, float StartX, float StartY, float StartZ, float Time)
{
    return new AParticleTrackingRecord(ParticleId, StartEnergy, StartX, StartY, StartZ, Time);
}

AParticleTrackingRecord *AParticleTrackingRecord::create()
{
    return new AParticleTrackingRecord(-1, 0, 0, 0, 0, 0);
}

void AParticleTrackingRecord::update(int particleId, float startEnergy, float startX, float startY, float startZ, float time)
{
    ParticleId       = particleId;
    StartEnergy      = startEnergy;
    StartPosition[0] = startX;
    StartPosition[1] = startY;
    StartPosition[2] = startZ;
    StartTime        = time;
}

AParticleTrackingRecord *AParticleTrackingRecord::create(int ParticleId, float StartEnergy, float *StartPosition, float Time)
{
    return new AParticleTrackingRecord(ParticleId, StartEnergy, StartPosition, Time);
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

const QString AParticleTrackingRecord::toString(int offset, const QStringList & ParticleNames, bool bExpandSecondaries) const
{
    QString s = QString(' ').repeated(offset) + '>';
    s += (ParticleId > -1 && ParticleId < ParticleNames.size() ? ParticleNames.at(ParticleId) : "unknown");
    s += QString("  E=%1  Pos=(%2,%3,%4)  Time=%5\n").arg(StartEnergy).arg(StartPosition[0]).arg(StartPosition[1]).arg(StartPosition[2]).arg(StartTime);

    //offset += 2;
    size_t SecIndex = 0;
    for (auto* st : Steps)
    {
        s += st->toString(offset);
        if (bExpandSecondaries)
        {
            const size_t numSec = st->Secondaries.size();
            for (size_t iSec = 0; iSec<numSec; iSec++)
            {
                if (SecIndex < Secondaries.size())
                {
                    s += Secondaries.at(SecIndex)->toString(offset + 2, ParticleNames, bExpandSecondaries);
                    SecIndex++;
                }
                else qDebug() << "Wrong secondary index SecIndex detected!";
            }
        }
    }
    return s;
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
