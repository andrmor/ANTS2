#include "apmdummystructure.h"
#include "ajsontools.h"

#include <QDebug>

void APMdummyStructure::writeToJson(QJsonObject & json) const
{
    QJsonObject js;

    js["Type"] = PMtype;
    js["Array"] = UpperLower;

    QJsonArray arrR;
        arrR.append(r[0]);
        arrR.append(r[1]);
        arrR.append(r[2]);
    js["R"] = arrR;

    QJsonArray arrA;
        arrA.append(Angle[0]);
        arrA.append(Angle[1]);
        arrA.append(Angle[2]);
    js["Angle"] = arrA;

    json["DummyPM"] = js;
}

bool APMdummyStructure::readFromJson(const QJsonObject & json)
{
    if (!json.contains("DummyPM"))
    {
        qWarning() << "Not a dummy pm json!";
        return false;
    }

    QJsonObject js = json["DummyPM"].toObject();

    parseJson(js, "Type", PMtype);
    parseJson(js, "Array", UpperLower);

    QJsonArray arrR = js["R"].toArray();
    r[0] = arrR[0].toDouble();
    r[1] = arrR[1].toDouble();
    r[2] = arrR[2].toDouble();

    QJsonArray arrA = js["Angle"].toArray();
    Angle[0] = arrA[0].toDouble();
    Angle[1] = arrA[1].toDouble();
    Angle[2] = arrA[2].toDouble();

    return true;
}
