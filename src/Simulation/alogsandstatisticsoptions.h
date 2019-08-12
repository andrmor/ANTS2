#ifndef ALOGSANDSTATISTICSOPTIONS_H
#define ALOGSANDSTATISTICSOPTIONS_H

class QJsonObject;

class ALogsAndStatisticsOptions
{
public:
    bool bParticleTransportLog = false;

    bool bPhotonDetectionStat = false;
    bool bPhotonProcessesStat = false;
    bool bPhotonGenerationLog = false;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);
};

#endif // ALOGSANDSTATISTICSOPTIONS_H
