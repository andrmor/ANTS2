#ifndef AG4SIMULATIONSETTINGS_H
#define AG4SIMULATIONSETTINGS_H

#include <QString>
#include <QStringList>

class QJsonObject;

class AG4SimulationSettings
{
public:
    AG4SimulationSettings();

    bool bTrackParticles = false;

    QStringList SensitiveVolumes; //later will be filled automatically
    QStringList Commands;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);

    const QString getPrimariesFileName(int iThreadNum) const;
    const QString getDepositionFileName(int iThreadNum) const;
    const QString getReceitFileName(int iThreadNum) const;
    const QString getConfigFileName(int iThreadNum) const;
    const QString getGdmlFileName() const;

};

#endif // AG4SIMULATIONSETTINGS_H
