#ifndef APADPROPERTIES_H
#define APADPROPERTIES_H

#include "apadgeometry.h"

#include <TPad.h>

#include <QJsonObject>

class APadProperties
{
public:
    APadProperties();
    APadProperties(TPad *pad);

    void updatePadGeometry();
    void applyPadGeometry();

    void writeToJson(QJsonObject &json) const;
    void readFromJson(const QJsonObject &json);

    QString toString() const;

    TPad * tPad = nullptr;
    APadGeometry padGeo;
};

#endif // APADPROPERTIES_H
