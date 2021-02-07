#include "apadgeometry.h"
#include "ajsontools.h"

APadGeometry::APadGeometry()
{

}

void APadGeometry::writeToJson(QJsonObject &json)
{
    json["xLow"]  = xLow;
    json["yLow"]  = yLow;
    json["xHigh"] = xHigh;
    json["yHigh"] = yHigh;
}

void APadGeometry::readFromJson(QJsonObject &json)
{
    parseJson(json, "xLow",  xLow);
    parseJson(json, "yLow",  yLow);
    parseJson(json, "xHigh", xHigh);
    parseJson(json, "yHigh", yHigh);

}
