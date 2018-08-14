#include "atrackbuildoptions.h"
#include "ajsontools.h"

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
    json["GeneralPhoton_Attributes"] = TA_Photons.writeToJson();

    json["PhotonSpecialRule_HittingPMs"] = bPhotonSpecialRule_HittingPMs;
    json["PhotonHittingPM_Attributes"] = TA_PhotonsHittingPMs.writeToJson();

    json["PhotonSpecialRule_SecScint"] = bPhotonSpecialRule_SecScint;
    json["PhotonSecScint_Attributes"] = TA_PhotonsSecScint.writeToJson();

    json["SkipPhotonsMissingPMs"] = bSkipPhotonsMissingPMs;
}

void ATrackBuildOptions::readFromJson(const QJsonObject &json)
{
    clear();
    if (json.isEmpty()) return;

    QJsonObject js;
    parseJson(json, "GeneralPhoton_Attributes", js);
    TA_Photons.readFromJson(js);

    parseJson(json, "PhotonSpecialRule_HittingPMs", bPhotonSpecialRule_HittingPMs);
    parseJson(json, "PhotonHittingPM_Attributes", js);
    TA_PhotonsHittingPMs.readFromJson(js);

    parseJson(json, "PhotonSpecialRule_SecScint", bPhotonSpecialRule_SecScint);
    parseJson(json, "PhotonSecScint_Attributes", js);
    TA_PhotonsSecScint.readFromJson(js);

    parseJson(json, "SkipPhotonsMissingPMs", bSkipPhotonsMissingPMs);
}

void ATrackBuildOptions::clear()
{
    TA_Photons.reset();

    TA_PhotonsHittingPMs.reset();
    bPhotonSpecialRule_HittingPMs = true;
    TA_PhotonsHittingPMs.color = 2;

    TA_PhotonsSecScint.reset();
    bPhotonSpecialRule_SecScint = true;
    TA_PhotonsSecScint.color = 6;

    bSkipPhotonsMissingPMs = false;
}
