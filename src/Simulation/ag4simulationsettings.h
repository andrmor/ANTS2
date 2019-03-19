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
    QStringList Commands = {"/process/em/fluo true", "/process/em/auger true", "/process/em/pixe true", "/run/setCut 0.0001 mm"};
    int Seed;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);

    const QString getPrimariesFileName(int iThreadNum);
    const QString getDepositionFileName(int iThreadNum);
    const QString getReceitFileName(int iThreadNum);
    const QString getConfigFileName(int iThreadNum);
    const QString getGdmlFileName();

};

#endif // AG4SIMULATIONSETTINGS_H
