#ifndef AISOTOPE_H
#define AISOTOPE_H

#include <QString>

class TString;
class QJsonObject;

class AIsotope
{
public:
    QString Symbol;
    int Mass;
    double Abundancy;  //in %

    AIsotope(const QString Symbol, int Mass, double Abundancy) : Symbol(Symbol), Mass(Mass), Abundancy(Abundancy) {}
    AIsotope() : Symbol("Undefined"), Mass(777), Abundancy(0) {}

    const TString getTName() const;

    void writeToJson(QJsonObject &json) const;
    void readFromJson(const QJsonObject &json);
};

#endif // AISOTOPE_H
