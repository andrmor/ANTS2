#include "aeventtrackingrecord.h"

#include <QStringList>
#include <QDebug>

// ============= Step ==============

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

AParticleTrackingRecord::AParticleTrackingRecord(int ParticleId, float StartEnergy, float * StartPosition, float Time, AParticleTrackingRecord * SecondaryOf) :
    ParticleId(ParticleId), StartEnergy(StartEnergy), StartTime(Time), SecondaryOf(SecondaryOf)
{
    for (int i=0; i<3; i++)
        this->StartPosition[i] = StartPosition[i];
}

AParticleTrackingRecord::~AParticleTrackingRecord()
{
    for (auto* step : Steps) delete step;
    for (auto* sec  : Secondaries) delete sec;
}

AParticleTrackingRecord *AParticleTrackingRecord::create(int ParticleId, double StartEnergy, double * StartPosition, double Time, AParticleTrackingRecord * SecondaryOf)
{
    float pos[3];
    for (int i=0; i<3; i++) pos[i] = static_cast<float>(StartPosition[i]);

    return new AParticleTrackingRecord(ParticleId,
                                       static_cast<float>(StartEnergy),
                                       pos,
                                       static_cast<float>(Time),
                                       SecondaryOf);
}

AParticleTrackingRecord *AParticleTrackingRecord::create(int ParticleId, float StartEnergy, float *StartPosition, float Time, AParticleTrackingRecord *SecondaryOf)
{
    return new AParticleTrackingRecord(ParticleId, StartEnergy, StartPosition, Time, SecondaryOf);
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

    offset += 2;
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
                    s += Secondaries.at(SecIndex)->toString(offset, ParticleNames, bExpandSecondaries);
                    SecIndex++;
                }
                else qDebug() << "Wrong secondary index SecIndex detected!";
            }
        }
    }
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

int AEventTrackingRecord::countPrimaries() { return static_cast<int>(PrimaryParticleRecords.size()); }
