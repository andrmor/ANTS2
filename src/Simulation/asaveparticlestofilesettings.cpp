#include "asaveparticlestofilesettings.h"
#include "ajsontools.h"

void ASaveParticlesToFileSettings::writeToJson(QJsonObject & json) const
{
    json["SaveParticles"] = SaveParticles;
    json["VolumeName"]    = VolumeName;
    json["FileName"]      = FileName;
    json["UseBinary"]     = UseBinary;
    json["UseTimeWindow"] = UseTimeWindow;
    json["TimeFrom"]      = TimeFrom;
    json["TimeTo"]        = TimeTo;
    json["StopTrack"]     = StopTrack;
}

void ASaveParticlesToFileSettings::readFromJson(const QJsonObject & json)
{
    SaveParticles = false;
    parseJson(json, "SaveParticles", SaveParticles);
    parseJson(json, "VolumeName",    VolumeName);
    parseJson(json, "UseBinary",     UseBinary);
    parseJson(json, "FileName",      FileName);
    parseJson(json, "UseTimeWindow", UseTimeWindow);
    parseJson(json, "TimeFrom",      TimeFrom);
    parseJson(json, "TimeTo",        TimeTo);
    parseJson(json, "StopTrack",     StopTrack);
}
