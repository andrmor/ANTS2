#ifndef AG4SIMULATIONSETTINGS_H
#define AG4SIMULATIONSETTINGS_H

#include <QString>
#include <QStringList>
#include <QMap>

class QJsonObject;

class AG4SimulationSettings
{
public:
    AG4SimulationSettings();

    bool bTrackParticles = false;

    QString               PhysicsList = "QGSP_BERT_HP";
    QStringList           SensitiveVolumes;
    QStringList           Commands;
    QMap<QString, double> StepLimits;
    bool                  BinaryOutput = false;
    int                   Precision = 6;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);

    const QString getPrimariesFileName(int iThreadNum) const;
    const QString getDepositionFileName(int iThreadNum) const;
    const QString getReceitFileName(int iThreadNum) const;
    const QString getConfigFileName(int iThreadNum) const;
    const QString getTracksFileName(int iThreadNum) const;
    const QString getMonitorDataFileName(int iThreadNum) const;
    const QString getExitParticleFileName(int iThreadNum) const;
    const QString getGdmlFileName() const;

    bool  checkPathValid() const;
    bool  checkExecutableExists() const;
    bool  checkExecutablePermission() const;

    const QString checkSensitiveVolumes() const;

private:
    const QString getPath() const;

};

#endif // AG4SIMULATIONSETTINGS_H
