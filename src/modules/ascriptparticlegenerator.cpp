#include "ascriptparticlegenerator.h"
#include "aparticlerecord.h"
#include "ajsontools.h"

AScriptParticleGenerator::AScriptParticleGenerator(const AMaterialParticleCollection &MpCollection) :
    MpCollection(MpCollection) {}

bool AScriptParticleGenerator::Init()
{
    return true;
}

void AScriptParticleGenerator::GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles)
{

}

bool AScriptParticleGenerator::IsParticleInUse(int particleId, QString &SourceNames) const
{
    return false; //TODO
}

void AScriptParticleGenerator::writeToJson(QJsonObject &json) const
{
    json["Script"] = Script;
}

bool AScriptParticleGenerator::readFromJson(const QJsonObject &json)
{
    return parseJson(json, "Script", Script);
}
