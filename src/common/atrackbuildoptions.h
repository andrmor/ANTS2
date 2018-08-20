#ifndef ATRACKDRAWOPTIONS_H
#define ATRACKDRAWOPTIONS_H

#include <QVector>

class QJsonObject;
class TrackHolderClass;

class ATrackAttributes
{
public:
    int color = 7;
    int width = 1;
    int style = 1;

    void writeToJson(QJsonObject& json) const;
    const QJsonObject writeToJson() const;
    void readFromJson(const QJsonObject& json);

    void setTrackAttributes(TrackHolderClass* track) const;

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
    bool bSkipPrimariesNoInteraction = false;
    bool bSkipSecondaries = false;

    int MaxPhotonTracks = 1000;
    int MaxParticleTracks = 1000;

    //JSON
    void writeToJson(QJsonObject& json) const;
    void readFromJson(const QJsonObject& json);

    void applyToParticleTrack(TrackHolderClass* track, int ParticleId) const;

private:
    void clear(); //clear and reset to default values
    void clearCustomParticleAttributes();
};

#endif // ATRACKDRAWOPTIONS_H
