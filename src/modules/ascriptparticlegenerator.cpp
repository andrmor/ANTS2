#include "ascriptparticlegenerator.h"

AScriptParticleGenerator::AScriptParticleGenerator()
{

}

bool AScriptParticleGenerator::Init()
{
    return true;
}

QVector<AGeneratedParticle> *AScriptParticleGenerator::GenerateEvent()
{
    QVector<AGeneratedParticle>* GeneratedParticles = new QVector<AGeneratedParticle>;

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
