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
    TPad *tPad = nullptr;
    APadGeometry padGeo;
    int basketIndex = 0;

    void updatePadGeometry();
    void applyPadGeometry();
    void writeToJson(QJsonObject &json) const;
    void readFromJson(const QJsonObject &json);
    QString toString();
};

#endif // APADPROPERTIES_H
