#include "ag4simulationsettings.h"
#include "aglobalsettings.h"
#include "ajsontools.h"

#include <QJsonObject>
#include <QJsonArray>

AG4SimulationSettings::AG4SimulationSettings()
{
    Commands = QStringList({"/process/em/fluo true",
                            "/process/em/auger true",
                            "/process/em/augerCascade true",
                            "/process/em/pixe true",
                            "/run/setCut 0.01 mm"});
}

void AG4SimulationSettings::writeToJson(QJsonObject &json) const
{
    json["TrackParticles"] = bTrackParticles;

    json["PhysicsList"] = PhysicsList;

    QJsonArray arSV;
    for (auto & v : SensitiveVolumes)
        arSV.append(v);
    json["SensitiveVolumes"] = arSV;

    QJsonArray arC;
    for (auto & c : Commands)
        arC.append(c);
    json["Commands"] = arC;

    QJsonArray arSL;
    for (auto & key : StepLimits.keys())
    {
        QJsonArray el;
        el << key << StepLimits.value(key);
        arSL.push_back(el);
    }
    json["StepLimits"] = arSL;

    json["Precision"]         = Precision;
}

void AG4SimulationSettings::readFromJson(const QJsonObject &json)
{
    parseJson(json, "TrackParticles", bTrackParticles);

    parseJson(json, "PhysicsList", PhysicsList);

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

    QJsonArray arSL;
    StepLimits.clear();
    parseJson(json, "StepLimits", arSL);
    for (int i=0; i<arSL.size(); i++)
    {
        QJsonArray el = arSL[i].toArray();
        if (el.size() > 1)
        {
            QString vol = el[0].toString();
            double step = el[1].toDouble();
            StepLimits[vol] = step;
        }
    }

    parseJson(json, "Precision", Precision);
}

const QString AG4SimulationSettings::getPrimariesFileName(int iThreadNum) const
{
    return getPath() + QString("primaries-%1.txt").arg(iThreadNum);
}

const QString AG4SimulationSettings::getDepositionFileName(int iThreadNum) const
{
    return getPath() + QString("deposition-%1.txt").arg(iThreadNum);
}

const QString AG4SimulationSettings::getReceitFileName(int iThreadNum) const
{
    return getPath() + QString("receipt-%1.json").arg(iThreadNum);
}

const QString AG4SimulationSettings::getConfigFileName(int iThreadNum) const
{
    return getPath() + QString("aga-%1.json").arg(iThreadNum);
}

const QString AG4SimulationSettings::getTracksFileName(int iThreadNum) const
{
    return getPath() + QString("tracks-%1.txt").arg(iThreadNum);
}

const QString AG4SimulationSettings::getGdmlFileName() const
{
    return getPath() + "Detector.gdml";
}

const QString AG4SimulationSettings::getPath() const
{
    QString Path = AGlobalSettings::getInstance().G4ExchangeFolder;
    if (!Path.endsWith('/')) Path += '/';
    return Path;
}
