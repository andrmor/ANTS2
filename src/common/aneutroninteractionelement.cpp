#include "aneutroninteractionelement.h"
#include "ajsontools.h"
#include "amaterialparticlecolection.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

void AAbsorptionGeneratedParticle::writeToJson(QJsonObject &json, AMaterialParticleCollection* MpCollection) const
{
    if (ParticleId<0 || ParticleId>=MpCollection->countParticles())
    {
        qWarning() << "Attempt to save capture secondary particle with wrong particleId!";
        return;
    }
    json["Particle"] = MpCollection->getParticle(ParticleId)->writeToJson();
    json["Energy"] = Energy;
    json["OpositeDirectionWithPrevious"] = bOpositeDirectionWithPrevious;
}

const QJsonObject AAbsorptionGeneratedParticle::writeToJson(AMaterialParticleCollection *MpCollection) const
{
    QJsonObject js;
    writeToJson(js, MpCollection);
    return js;
}

void AAbsorptionGeneratedParticle::readFromJson(const QJsonObject &json, AMaterialParticleCollection *MpCollection)
{
    QJsonObject jsp = json["Particle"].toObject();
    ParticleId = MpCollection->findOrCreateParticle(jsp);
    parseJson(json, "Energy", Energy);
    parseJson(json, "OpositeDirectionWithPrevious", bOpositeDirectionWithPrevious);
}

void ADecayScenario::writeToJson(QJsonObject &json, AMaterialParticleCollection *MpCollection) const
{
    json["Branching"] = Branching;

    QJsonArray ar;
    for (const AAbsorptionGeneratedParticle& p : GeneratedParticles)
        ar << p.writeToJson(MpCollection);
    json["GeneratedParticles"] = ar;
}

const QJsonObject ADecayScenario::writeToJson(AMaterialParticleCollection *MpCollection) const
{
    QJsonObject js;
    writeToJson(js, MpCollection);
    return js;
}

void ADecayScenario::readFromJson(const QJsonObject &json, AMaterialParticleCollection *MpCollection)
{
    parseJson(json, "Branching", Branching);

    GeneratedParticles.clear();
    QJsonArray ar = json["GeneratedParticles"].toArray();
    for (int i=0; i<ar.size(); i++)
    {
        QJsonObject js = ar[i].toObject();
        AAbsorptionGeneratedParticle jp;
        jp.readFromJson(js, MpCollection);
        GeneratedParticles << jp;
    }
}

void ANeutronInteractionElement::writeToJson(QJsonObject &json, AMaterialParticleCollection *MpCollection) const
{
    json["Name"] = Name;
    json["Mass"] = Mass;
    json["MolarFraction"] = MolarFraction;

    QJsonArray ar;
    writeTwoQVectorsToJArray(Energy, CrossSection, ar);
    json["CrossSection"] = ar;

    json["Header"] = CSfileHeader;

    QJsonArray crArr;
    for (const ADecayScenario& cr : DecayScenarios)
        crArr << cr.writeToJson(MpCollection);
    json["DecayScenarios"] = crArr;
}

const QJsonObject ANeutronInteractionElement::writeToJson(AMaterialParticleCollection *MpCollection) const
{
    QJsonObject json;
    writeToJson(json, MpCollection);
    return json;
}

void ANeutronInteractionElement::readFromJson(const QJsonObject &json, AMaterialParticleCollection *MpCollection)
{
    parseJson(json, "Name", Name);
    parseJson(json, "Mass", Mass);
    parseJson(json, "MolarFraction", MolarFraction);

    Energy.clear();
    CrossSection.clear();
    QJsonArray ar = json["CrossSection"].toArray();
    readTwoQVectorsFromJArray(ar, Energy, CrossSection);

    CSfileHeader.clear();
    parseJson(json, "Header", CSfileHeader);

    DecayScenarios.clear();
    ar = json["DecayScenarios"].toArray();
    for (int i=0; i<ar.size(); i++)
    {
        QJsonObject js = ar[i].toObject();
        ADecayScenario r;
        r.readFromJson(js, MpCollection);
        DecayScenarios << r;
    }
}
