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
    ParticleId = MpCollection->findOrAddParticle(jsp);
    parseJson(json, "Energy", Energy);
    parseJson(json, "OpositeDirectionWithPrevious", bOpositeDirectionWithPrevious);
}

void ADecayScenario::writeToJson(QJsonObject &json, AMaterialParticleCollection *MpCollection) const
{
    json["Branching"] = Branching;

    json["InteractionModel"] = static_cast<int>(InteractionModel);

    QJsonArray ar;
    for (const AAbsorptionGeneratedParticle& p : GeneratedParticles)
        ar << p.writeToJson(MpCollection);
    json["GeneratedParticles"] = ar;

    json["DirectModel"] = static_cast<int>(DirectModel);
    json["DirectConst"]   = DirectConst;
    json["DirectAverage"] = DirectAverage;
    json["DirectSigma"]   = DirectSigma;

    QJsonArray arc;
    for (int i=0; i<DirectCustomEn.size() && i<DirectCustomProb.size(); i++)
    {
        QJsonArray el;
        el << DirectCustomEn.at(i) << DirectCustomProb.at(i);
        arc.push_back(el);
    }
    json["DirectCustom"] = arc;
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

    //compatibility
    InteractionModel = ( GeneratedParticles.isEmpty() ? Loss : FissionFragments ); //compatibility

    //new settings
    if (json.contains("InteractionModel"))
    {
        int val = 0;
        parseJson(json, "InteractionModel", val);
        if (val >= 0 && val < 3) InteractionModel = static_cast<eInteractionModels>(val);
    }
    if (json.contains("DirectModel"))
    {
        int val = 0;
        parseJson(json, "DirectModel", val);
        if (val >= 0 && val < 3) DirectModel = static_cast<eDirectModels>(val);
    }
    parseJson(json, "DirectConst", DirectConst);
    parseJson(json, "DirectAverage", DirectAverage);
    parseJson(json, "DirectSigma", DirectSigma);
    QJsonArray arc;
    parseJson(json, "DirectCustom", arc);
    for (int i=0; i<arc.size(); i++)
    {
        QJsonArray el = arc[i].toArray();
        if (el.size() > 1)
        {
            DirectCustomEn   << el[0].toDouble();
            DirectCustomProb << el[1].toDouble();
        }
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
    QJsonArray ar;
    parseJson(json, "CrossSection", ar);
    readTwoQVectorsFromJArray(ar, Energy, CrossSection);

    CSfileHeader.clear();
    parseJson(json, "Header", CSfileHeader);

    readScenariosFromJson(json, MpCollection);
}

void ANeutronInteractionElement::readScenariosFromJson(const QJsonObject &json, AMaterialParticleCollection *MpCollection)
{
    DecayScenarios.clear();
    QJsonArray ar;
    parseJson(json, "DecayScenarios", ar);
    for (int i=0; i<ar.size(); i++)
    {
        QJsonObject js = ar[i].toObject();
        ADecayScenario r;
        r.readFromJson(js, MpCollection);
        DecayScenarios << r;
    }
}

#include "TH1.h"
void ANeutronInteractionElement::updateRuntimeProperties()
{
    for (ADecayScenario & ds : DecayScenarios)
    {
        delete ds.DirectCustomDist; ds.DirectCustomDist = 0;

        int size = ds.DirectCustomEn.size();
        if (size > 0)
        {
            double first = ds.DirectCustomEn.first();
            double last  = ds.DirectCustomEn.last();
            double delta = ( size > 1 ? (last - first) / size : 1.0);
            ds.DirectCustomDist = new TH1D("", "Custom deposition", size, first, last + delta);
            for (int j = 1; j<size+1; j++)
                ds.DirectCustomDist->SetBinContent(j, ds.DirectCustomProb.at(j-1));
            ds.DirectCustomDist->ComputeIntegral(true); //to make thread safe
        }
    }
}
