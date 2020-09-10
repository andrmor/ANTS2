#include "asimsettings.h"
#include "ajsontools.h"

#include <QDebug>

void ASimSettings::writeToJson(QJsonObject & json) const
{
    QJsonObject jsim;

    jsim["Mode"] = (bOnlyPhotons ? "PointSim" : "SourceSim");

    genSimSet.writeToJson(jsim);

    {
        QJsonObject js;
            photSimSet.writeToJson(js);
        jsim["PointSourcesConfig"] = js;
    }

    {
        QJsonObject js;
            partSimSet.writeToJson(js);
        jsim["ParticleSourcesConfig"] = js;
    }

    json["SimulationConfig"] = jsim;
}

bool ASimSettings::readFromJson(const QJsonObject & json)
{
    QJsonObject js;

    bool ok = parseJson(json, "SimulationConfig", js);
    if (!ok)
    {
        qWarning() << "SimulationConfig is absent";
        return false;
    }

    const QString mode = js["Mode"].toString();
    bOnlyPhotons = (mode == "PointSim");

    ok = genSimSet.readFromJson(js);
    if (!ok) return false;

    QJsonObject phjs;
    ok = parseJson(js, "PointSourcesConfig", phjs);
    if (!ok) return false;
    photSimSet.readFromJson(phjs);

    QJsonObject pajs;
    ok = parseJson(js, "ParticleSourcesConfig", pajs);
    if (!ok) return false;
    partSimSet.readFromJson(pajs);

    return true;
}
