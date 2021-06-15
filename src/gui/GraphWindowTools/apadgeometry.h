#ifndef APADGEOMETRY_H
#define APADGEOMETRY_H

class QJsonObject;

class APadGeometry
{
public:
    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);

    double xLow;
    double yLow;
    double xHigh;
    double yHigh;
};

#endif // APADGEOMETRY_H
