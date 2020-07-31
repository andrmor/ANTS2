#include "ageoconsts.h"
#include "ajsontools.h"

AGeoConsts &AGeoConsts::getInstance()
{
    static AGeoConsts instance;
    return instance;
}

const AGeoConsts &AGeoConsts::getConstInstance()
{
    return getInstance();
}

void AGeoConsts::clearConstants()
{
    geoConsts.clear();
}

void AGeoConsts::writeToJson(QJsonObject & json) const
{
    QJsonArray ar;

    QMapIterator<QString, double> iter(geoConsts);
    while (iter.hasNext())
    {
        iter.next();
        QJsonArray el;
            el << iter.key() << iter.value();
        ar.push_back(el);
    }

    json["Map"] = ar;
}

void AGeoConsts::readFromJson(const QJsonObject & json)
{
    clearConstants();

    QJsonArray ar;
    parseJson(json, "Map", ar);

    for (int iA = 0; iA < ar.size(); iA++)
    {
        QJsonArray el = ar[iA].toArray();
        if (el.size() >= 2)
        {
            const QString key = el[0].toString();
            const double  val = el[1].toDouble();
            geoConsts[key] = val;      // warnings if not unique?
        }
    }
}

AGeoConsts::AGeoConsts()
{
//    geoConsts.insert("x", 50);
}
