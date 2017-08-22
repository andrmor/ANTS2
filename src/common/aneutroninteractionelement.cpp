#include "aneutroninteractionelement.h"
#include "ajsontools.h"
#include "amaterialparticlecolection.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

// --- base ---

void ANeutronInteractionElement::writeToJson(QJsonObject &json) const
{
    json["Name"] = Name;
    json["Mass"] = Mass;
    json["MolarFraction"] = MolarFraction;

    QJsonArray ar;
    writeTwoQVectorsToJArray(Energy, CrossSection, ar);
    json["CrossSection"] = ar;
}

void ANeutronInteractionElement::readFromJson(const QJsonObject &json)
{
    parseJson(json, "Name", Name);
    parseJson(json, "Mass", Mass);
    parseJson(json, "MolarFraction", MolarFraction);

    Energy.clear();
    CrossSection.clear();
    QJsonArray ar = json["CrossSection"].toArray();
    readTwoQVectorsFromJArray(ar, Energy, CrossSection);
}

// --- elastic ---

void AElasticScatterElement::writeToJson(QJsonObject &json) const
{
   ANeutronInteractionElement::writeToJson(json);
}

const QJsonObject AElasticScatterElement::writeToJson()
{
    QJsonObject json;
    AElasticScatterElement::writeToJson(json);
    return json;
}

void AElasticScatterElement::readFromJson(const QJsonObject &json)
{
    ANeutronInteractionElement::readFromJson(json);
}

// --- capture ---

void ACaptureGeneratedParticle::writeToJson(QJsonObject &json, AMaterialParticleCollection* MpCollection) const
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

const QJsonObject ACaptureGeneratedParticle::writeToJson(AMaterialParticleCollection *MpCollection) const
{
    QJsonObject js;
    writeToJson(js, MpCollection);
    return js;
}

void ACaptureGeneratedParticle::readFromJson(const QJsonObject &json, AMaterialParticleCollection *MpCollection)
{
    QJsonObject jsp = json["Particle"].toObject();
    ParticleId = MpCollection->findOrCreateParticle(jsp);
    parseJson(json, "Energy", Energy);
    parseJson(json, "OpositeDirectionWithPrevious", bOpositeDirectionWithPrevious);
}

void ACaptureReaction::writeToJson(QJsonObject &json, AMaterialParticleCollection *MpCollection) const
{
    json["Branching"] = Branching;

    QJsonArray ar;
    for (const ACaptureGeneratedParticle& p : GeneratedParticles)
        ar << p.writeToJson(MpCollection);
    json["GeneratedParticles"] = ar;
}

const QJsonObject ACaptureReaction::writeToJson(AMaterialParticleCollection *MpCollection) const
{
    QJsonObject js;
    writeToJson(js, MpCollection);
    return js;
}

void ACaptureReaction::readFromJson(const QJsonObject &json, AMaterialParticleCollection *MpCollection)
{
    parseJson(json, "Branching", Branching);

    GeneratedParticles.clear();
    QJsonArray ar = json["GeneratedParticles"].toArray();
    for (int i=0; i<ar.size(); i++)
    {
        QJsonObject js = ar[i].toObject();
        ACaptureGeneratedParticle jp;
        jp.readFromJson(js, MpCollection);
        GeneratedParticles << jp;
    }
}

void AAbsorptionElement::writeToJson(QJsonObject &json, AMaterialParticleCollection *MpCollection) const
{
    ANeutronInteractionElement::writeToJson(json);

    QJsonArray crArr;
    for (const ACaptureReaction& cr : Reactions)
        crArr << cr.writeToJson(MpCollection);
    json["Reactions"] = crArr;
}

const QJsonObject AAbsorptionElement::writeToJson(AMaterialParticleCollection *MpCollection)
{
    QJsonObject js;
    writeToJson(js, MpCollection);
    return js;
}

void AAbsorptionElement::readFromJson(QJsonObject &json, AMaterialParticleCollection *MpCollection)
{
    ANeutronInteractionElement::readFromJson(json);

    Reactions.clear();
    QJsonArray ar = json["Reactions"].toArray();
    for (int i=0; i<ar.size(); i++)
    {
        QJsonObject js = ar[i].toObject();
        ACaptureReaction r;
        r.readFromJson(js, MpCollection);
        Reactions << r;
    }
}
