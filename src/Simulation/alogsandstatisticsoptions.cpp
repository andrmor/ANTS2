#include "alogsandstatisticsoptions.h"
#include "ajsontools.h"

#include <QJsonObject>

void ALogsAndStatisticsOptions::writeToJson(QJsonObject &json) const
{
    json["ParticleTransportLog"] = bParticleTransportLog;
        json["bSaveParticleLog"] = bSaveParticleLog;
    json["bSaveDepositionLog"]   = bSaveDepositionLog;

    json["PhotonDetectionStat"]  = bPhotonDetectionStat;
    json["PhotonGenerationLog"]  = bPhotonGenerationLog;
}

void ALogsAndStatisticsOptions::readFromJson(const QJsonObject &json)
{
    bParticleTransportLog = false;
    parseJson(json, "ParticleTransportLog", bParticleTransportLog);
    bSaveParticleLog = false;
    parseJson(json, "bSaveParticleLog", bSaveParticleLog);
    bSaveDepositionLog = false;
    parseJson(json, "bSaveDepositionLog", bSaveDepositionLog);

    bPhotonDetectionStat  = false;
    parseJson(json, "PhotonDetectionStat",  bPhotonDetectionStat);
    bPhotonGenerationLog  = false;
    parseJson(json, "PhotonGenerationLog",  bPhotonGenerationLog);
}
