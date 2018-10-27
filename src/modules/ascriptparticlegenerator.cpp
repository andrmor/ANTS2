#include "ascriptparticlegenerator.h"

AScriptParticleGenerator::AScriptParticleGenerator()
{

}

bool AScriptParticleGenerator::Init()
{
    return true;
}

QVector<AParticleRecord> *AScriptParticleGenerator::GenerateEvent()
{
    QVector<AParticleRecord>* GeneratedParticles = new QVector<AParticleRecord>;

    return GeneratedParticles;
}

void AScriptParticleGenerator::RemoveParticle(int particleId)
{
    //TODO
}

bool AScriptParticleGenerator::IsParticleInUse(int particleId, QString &SourceNames) const
{
    return false; //TODO
}
