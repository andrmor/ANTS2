#ifndef ATRACKDRAWOPTIONS_H
#define ATRACKDRAWOPTIONS_H

#include <QVector>

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

    void reset();
};

class ATrackBuildOptions
{
public:
    ATrackBuildOptions();

    bool bBuildPhotonTracks = false;
    bool bBuildParticleTracks = false;

    //Photons
    ATrackAttributes TA_Photons;

    bool bPhotonSpecialRule_HittingPMs = true;
    ATrackAttributes TA_PhotonsHittingPMs;

    bool bPhotonSpecialRule_SecScint = true;
    ATrackAttributes TA_PhotonsSecScint;

    bool bSkipPhotonsMissingPMs = false;

    //Particles
    ATrackAttributes TA_DefaultParticle;  //default width/style and color for particle # beyound covered in DefaultParticle_Colors
    QVector<int> DefaultParticle_Colors;
    QVector<ATrackAttributes*> CustomParticle_Attributes;

    bool bSkipPrimaries = false;
    bool bSkipSecondaries = false;

    //JSON
    void writeToJson(QJsonObject& json) const;
    void readFromJson(const QJsonObject& json);

private:
    void clear(); //clear and reset to default values
    void clearCustomParticleAttributes();
};

#endif // ATRACKDRAWOPTIONS_H
