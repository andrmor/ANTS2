#include "ag4simulationsettings.h"
#include "aglobalsettings.h"
#include "ajsontools.h"

#include <QJsonObject>
#include <QJsonArray>

AG4SimulationSettings::AG4SimulationSettings()
{
    Commands = QStringList({"/process/em/fluo true", "/process/em/auger true", "/process/em/pixe true", "/run/setCut 0.01 mm"});
}

void AG4SimulationSettings::writeToJson(QJsonObject &json) const
{
    json[""] = bTrackParticles;

    json[""] = Seed;

    QJsonArray arSV;
    for (auto & v : SensitiveVolumes)
        arSV.append(v);
    json["SensitiveVolumes"] = arSV;

    QJsonArray arC;
    for (auto & c : Commands)
        arC.append(c);
    json["Commands"] = arC;

}

void AG4SimulationSettings::readFromJson(const QJsonObject &json)
{
    parseJson(json, "TrackParticles", bTrackParticles);
    parseJson(json, "Seed", Seed);

    QJsonArray arSV;
    SensitiveVolumes.clear();
    parseJson(json, "SensitiveVolumes", arSV);
    for (int i=0; i<arSV.size(); i++)
        SensitiveVolumes << arSV.at(i).toString();

    QJsonArray arC;
    Commands.clear();
    parseJson(json, "Commands", arC);
    for (int i=0; i<arC.size(); i++)
        Commands << arC.at(i).toString();

}

const QString AG4SimulationSettings::getPrimariesFileName(int iThreadNum)
{
    QString Path = AGlobalSettings::getInstance().G4ExchangeFolder;
    if (!Path.endsWith('/')) Path += '/';
    return Path + QString("primaries-%1.txt").arg(iThreadNum);
}

const QString AG4SimulationSettings::getDepositionFileName(int iThreadNum)
{
    QString Path = AGlobalSettings::getInstance().G4ExchangeFolder;
    if (!Path.endsWith('/')) Path += '/';
    return Path + QString("deposition-%1.txt").arg(iThreadNum);
}

const QString AG4SimulationSettings::getReceitFileName(int iThreadNum)
{
    QString Path = AGlobalSettings::getInstance().G4ExchangeFolder;
    if (!Path.endsWith('/')) Path += '/';
    return Path + QString("receipt-%1.txt").arg(iThreadNum);
}

const QString AG4SimulationSettings::getConfigFileName(int iThreadNum)
{
    QString Path = AGlobalSettings::getInstance().G4ExchangeFolder;
    if (!Path.endsWith('/')) Path += '/';
    return Path + QString("aga-%1.txt").arg(iThreadNum);
}

const QString AG4SimulationSettings::getGdmlFileName()
{
    QString Path = AGlobalSettings::getInstance().G4ExchangeFolder;
    if (!Path.endsWith('/')) Path += '/';
    return Path + "Detector.gdml";
}
