#include "atrackbuildoptions.h"
#include "ajsontools.h"
#include "atrackrecords.h"

#include "TVirtualGeoTrack.h"

void ATrackAttributes::writeToJson(QJsonObject &json) const
{
    json["color"] = color;
    json["width"] = width;
    json["style"] = style;
}

const QJsonObject ATrackAttributes::writeToJson() const
{
    QJsonObject json;
    writeToJson(json);
    return json;
}

void ATrackAttributes::readFromJson(const QJsonObject &json)
{
    parseJson(json, "color", color);
    parseJson(json, "width", width);
    parseJson(json, "style", style);
}

void ATrackAttributes::setTrackAttributes(TrackHolderClass *track) const
{
    track->Color = color;
    track->Width = width;
    track->Style = style;
}

void ATrackAttributes::setTrackAttributes(TVirtualGeoTrack *track) const
{
    track->SetLineColor(color);
    track->SetLineWidth(width);
    track->SetLineStyle(style);
}

void ATrackAttributes::reset()
{
    color = 7;
    width = 1;
    style = 1;
}

ATrackBuildOptions::ATrackBuildOptions()
{
    clear();
}

void ATrackBuildOptions::writeToJson(QJsonObject &json) const
{
    json["BuildPhotonTracks"] = bBuildPhotonTracks;
    json["BuildParticleTracks"] = bBuildParticleTracks;

    json["MaxPhotonTracks"] = MaxPhotonTracks;
    json["MaxParticleTracks"] = MaxParticleTracks;

    json["GeneralPhoton_Attributes"] = TA_Photons.writeToJson();

    json["PhotonSpecialRule_HittingPMs"] = bPhotonSpecialRule_HittingPMs;
    json["PhotonHittingPM_Attributes"] = TA_PhotonsHittingPMs.writeToJson();

    json["PhotonSpecialRule_SecScint"] = bPhotonSpecialRule_SecScint;
    json["PhotonSecScint_Attributes"] = TA_PhotonsSecScint.writeToJson();

    json["SkipPhotonsMissingPMs"] = bSkipPhotonsMissingPMs;

    // ----

    json["SkipParticles_Primary"] = bSkipPrimaries;
    json["SkipParticles_PrimaryNoInteraction"] = bSkipPrimariesNoInteraction;
    json["SkipParticles_Secondary"] = bSkipSecondaries;

    json["Particle_DefaultAttributes"] = TA_DefaultParticle.writeToJson();
    QJsonArray ar;
    for (const int& c : DefaultParticle_Colors) ar << c;
    json["Particle_DefaultColors"] = ar;
    ar = QJsonArray();
    for (ATrackAttributes* ta : CustomParticle_Attributes)
    {
        QJsonObject js;
        if (ta) ta->writeToJson(js);
        ar << js;
    }
    json["Particle_CustomAttribtes"] = ar;
}

void ATrackBuildOptions::readFromJson(const QJsonObject &json)
{
    clear();
    if (json.isEmpty()) return;

    parseJson(json, "BuildPhotonTracks", bBuildPhotonTracks);
    parseJson(json, "BuildParticleTracks", bBuildParticleTracks);

    parseJson(json, "MaxPhotonTracks", MaxPhotonTracks);
    parseJson(json, "MaxParticleTracks", MaxParticleTracks);

    QJsonObject js;
    parseJson(json, "GeneralPhoton_Attributes", js);
    TA_Photons.readFromJson(js);

    parseJson(json, "PhotonSpecialRule_HittingPMs", bPhotonSpecialRule_HittingPMs);
    js = QJsonObject();
    parseJson(json, "PhotonHittingPM_Attributes", js);
    TA_PhotonsHittingPMs.readFromJson(js);

    parseJson(json, "PhotonSpecialRule_SecScint", bPhotonSpecialRule_SecScint);
    js = QJsonObject();
    parseJson(json, "PhotonSecScint_Attributes", js);
    TA_PhotonsSecScint.readFromJson(js);

    parseJson(json, "SkipPhotonsMissingPMs", bSkipPhotonsMissingPMs);

    // -----

    parseJson(json, "SkipParticles_Primary", bSkipPrimaries);
    parseJson(json, "SkipParticles_PrimaryNoInteraction", bSkipPrimariesNoInteraction);
    parseJson(json, "SkipParticles_Secondary", bSkipSecondaries);

    js = QJsonObject();
    parseJson(json, "Particle_DefaultAttributes", js);
    TA_DefaultParticle.readFromJson(js);

    QJsonArray ar;
    parseJson(json, "Particle_DefaultColors", ar);
    DefaultParticle_Colors.clear();
    for (int i=0; i<ar.size(); i++)
        DefaultParticle_Colors << ar.at(i).toInt(1);

    clearCustomParticleAttributes();
    ar = QJsonArray();
    parseJson(json, "Particle_CustomAttribtes", ar);
    for (int i=0; i<ar.size(); i++)
    {
        QJsonObject js = ar.at(i).toObject();
        ATrackAttributes* ta = 0;
        if (!js.isEmpty())
        {
            ta = new ATrackAttributes();
            ta->readFromJson(js);
        }
        CustomParticle_Attributes << ta;
    }
}

void ATrackBuildOptions::applyToParticleTrack(TrackHolderClass *track, int ParticleId) const
{
    if ( ParticleId < CustomParticle_Attributes.size() && CustomParticle_Attributes.at(ParticleId) )
    {
        //custom properties defined for this particle
        CustomParticle_Attributes.at(ParticleId)->setTrackAttributes(track);
        return;
    }

    TA_DefaultParticle.setTrackAttributes(track);

    if ( ParticleId < DefaultParticle_Colors.size())
        track->Color = DefaultParticle_Colors.at(ParticleId);
}

void ATrackBuildOptions::applyToParticleTrack(TVirtualGeoTrack *track, int ParticleId) const
{
    if ( ParticleId < CustomParticle_Attributes.size() && CustomParticle_Attributes.at(ParticleId) )
    {
        //custom properties defined for this particle
        CustomParticle_Attributes.at(ParticleId)->setTrackAttributes(track);
        return;
    }

    TA_DefaultParticle.setTrackAttributes(track);

    if ( ParticleId < DefaultParticle_Colors.size())
        track->SetLineColor( DefaultParticle_Colors.at(ParticleId) );
}

int ATrackBuildOptions::getParticleColor(int ParticleId) const
{
    if ( ParticleId < CustomParticle_Attributes.size() && CustomParticle_Attributes.at(ParticleId) )
        return CustomParticle_Attributes.at(ParticleId)->color; //custom properties defined for this particle

    if ( ParticleId < DefaultParticle_Colors.size()) return DefaultParticle_Colors.at(ParticleId);
    else return TA_DefaultParticle.color;
}

void ATrackBuildOptions::clear()
{
    bBuildPhotonTracks = false;
    bBuildParticleTracks = false;

    TA_Photons.reset();

    TA_PhotonsHittingPMs.reset();
    bPhotonSpecialRule_HittingPMs = true;
    TA_PhotonsHittingPMs.color = 2;

    TA_PhotonsSecScint.reset();
    bPhotonSpecialRule_SecScint = true;
    TA_PhotonsSecScint.color = 6;

    bSkipPhotonsMissingPMs = false;

    //----particles----
    TA_DefaultParticle.color = 15;
    TA_DefaultParticle.width = 2;
    DefaultParticle_Colors.clear();
    DefaultParticle_Colors << 1 << 2 << 3 << 4 << 6 << 7 << 8 << 9 << 28 << 30 << 36 << 38 << 39 << 40 << 46 << 49;

    clearCustomParticleAttributes();
}

void ATrackBuildOptions::clearCustomParticleAttributes()
{
    for (ATrackAttributes* t : CustomParticle_Attributes)
        delete t;
    CustomParticle_Attributes.clear();
}
