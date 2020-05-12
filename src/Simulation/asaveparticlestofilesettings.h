#ifndef ASAVEPARTICLESTOFILESETTINGS_H
#define ASAVEPARTICLESTOFILESETTINGS_H

#include <QString>

class QJsonObject;

class ASaveParticlesToFileSettings
{
public:
    bool    SaveParticles = false;
    QString VolumeName;
    QString FileName;
    bool    UseBinary     = false;
    bool    UseTimeWindow = false;
    double  TimeFrom      = 0;
    double  TimeTo        = 1.0e6;
    bool    StopTrack     = true;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);
};

#endif // ASAVEPARTICLESTOFILESETTINGS_H
