#include "alogsandstatisticsoptions.h"
#include "ajsontools.h"

#include <QJsonObject>

void ALogsAndStatisticsOptions::writeToJson(QJsonObject &json) const
{
    json["ParticleTransportLog"] = bParticleTransportLog;

    json["PhotonDetectionStat"]  = bPhotonDetectionStat;
    json["PhotonProcessesStat"]  = bPhotonProcessesStat;
    json["PhotonGenerationLog"]  = bPhotonGenerationLog;
}

void ALogsAndStatisticsOptions::readFromJson(const QJsonObject &json)
{
    bParticleTransportLog = false;
    parseJson(json, "ParticleTransportLog", bParticleTransportLog);

    bPhotonDetectionStat  = false;
    bPhotonProcessesStat  = false;
    bPhotonGenerationLog  = false;
    parseJson(json, "PhotonDetectionStat",  bPhotonDetectionStat);
    parseJson(json, "PhotonProcessesStat",  bPhotonProcessesStat);
    parseJson(json, "PhotonGenerationLog",  bPhotonGenerationLog);
}
