#include "asimsettings.h"
#include "ajsontools.h"

#include <QDebug>

void ASimSettings::writeToJson(QJsonObject &json) const
{
    genSimSet.writeToJson(json);

    {
        QJsonObject js;
        photSimSet.writeToJson(js);
        json["PointSourcesConfig"] = js;
    }

    /*
    {
        QJsonObject js;
        partSimSet.writeToJson(js);
        json["ParticleSourcesConfig"] = js;
    }
    */
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

    ok = genSimSet.readFromJson(js);
    if (!ok) return false;

    QJsonObject phjs;
    ok = parseJson(js, "PointSourcesConfig", phjs);
    if (!ok) return false;
    photSimSet.readFromJson(phjs);

    /*
    QJsonObject pajs;
    ok = parseJson(js, "ParticleSourcesConfig", pajs);
    if (!ok) return false;
    photSimSet.readFromJson(pajs);
    */

    return true;
}
