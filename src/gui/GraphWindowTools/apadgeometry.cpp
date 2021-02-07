#include "apadgeometry.h"
#include "ajsontools.h"
#include <QDebug>

APadGeometry::APadGeometry()
{

}

void APadGeometry::writeToJson(QJsonObject &json) const
{
    json["xLow"]  = xLow;
    json["yLow"]  = yLow;
    json["xHigh"] = xHigh;
    json["yHigh"] = yHigh;
}

void APadGeometry::readFromJson(const QJsonObject &json)
{
    parseJson(json, "xLow",  xLow);
    parseJson(json, "yLow",  yLow);
    parseJson(json, "xHigh", xHigh);
    parseJson(json, "yHigh", yHigh);

    qDebug() <<"padGEometry" <<xLow <<yLow <<xHigh <<yHigh;

}
