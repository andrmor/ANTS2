#include "ascriptparticlegenerator.h"
#include "aparticlerecord.h"

AScriptParticleGenerator::AScriptParticleGenerator()
{

}

bool AScriptParticleGenerator::Init()
{
    return true;
}

void AScriptParticleGenerator::GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles)
{

}

void AScriptParticleGenerator::RemoveParticle(int particleId)
{
    //TODO
}

bool AScriptParticleGenerator::IsParticleInUse(int particleId, QString &SourceNames) const
{
    return false; //TODO
}
