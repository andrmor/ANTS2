#ifndef ACHEMICALELEMENT_H
#define ACHEMICALELEMENT_H

#include "aisotope.h"

#include <QString>
#include <QVector>

class QJsonObject;

class AChemicalElement
{
public:
    QString Symbol;
    QVector<AIsotope> Isotopes;
    double MolarFraction;

    AChemicalElement(const QString Symbol, double MolarFraction) : Symbol(Symbol), MolarFraction(MolarFraction) {}
    AChemicalElement() : Symbol("Undefined"), MolarFraction(0) {}

    const QString print() const;
    int countIsotopes() const {return Isotopes.size();}
    double getFractionWeight() const;

    void writeToJson(QJsonObject& json) const;
    void readFromJson(const QJsonObject& json);
};

#endif // ACHEMICALELEMENT_H
