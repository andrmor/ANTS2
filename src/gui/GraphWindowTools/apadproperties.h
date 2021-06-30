#ifndef APADPROPERTIES_H
#define APADPROPERTIES_H

#include "apadgeometry.h"

#include <QVector>
#include <QJsonObject>

#include "TPad.h"

class TObject;

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

    QVector<TObject*> tmpObjects;
};

#endif // APADPROPERTIES_H
