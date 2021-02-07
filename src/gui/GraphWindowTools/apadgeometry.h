#ifndef APADGEOMETRY_H
#define APADGEOMETRY_H


#include <QJsonObject>


class APadGeometry
{
public:
    APadGeometry();
    double xLow;
    double yLow;
    double xHigh;
    double yHigh;
    void writeToJson(QJsonObject &json);
    void readFromJson(QJsonObject &json);
};

#endif // APADGEOMETRY_H
