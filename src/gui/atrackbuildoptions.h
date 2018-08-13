#ifndef ATRACKDRAWOPTIONS_H
#define ATRACKDRAWOPTIONS_H

class QJsonObject;

class ATrackAttributes
{
public:
    int color = 7;
    int width = 1;
    int style = 1;

    void writeToJson(QJsonObject& json) const;
    const QJsonObject writeToJson() const;
    void readFromJson(const QJsonObject& json);
};

class ATrackBuildOptions
{
public:
    ATrackBuildOptions();

    //Photons
    ATrackAttributes TA_Photons;

    bool bPhotonSpecialRule_HittingPMs = true;
    ATrackAttributes TA_PhotonsHittingPMs;

    bool bPhotonSpecialRule_SecScint = true;
    ATrackAttributes TA_PhotonsSecScint;

    bool bSkipPhotonsMissingPMs = false;

    //Particles

    //JSON
    void writeToJson(QJsonObject& json) const;
    void readFromJson(const QJsonObject& json);
};

#endif // ATRACKDRAWOPTIONS_H
