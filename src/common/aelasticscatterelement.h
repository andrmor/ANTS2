#ifndef AELASTICSCATTERELEMENT_H
#define AELASTICSCATTERELEMENT_H

#include <QString>
#include <QVector>

class QJsonObject;

class AElasticScatterElement
{
public:
    QString Name;
    int Mass;
    double Abundancy;   // [0, 1]
    double Fraction;
    double StatWeight;  // = Abundancy * Fraction
    QVector<double> Energy;
    QVector<double> CrossSection;

    AElasticScatterElement(QString ElementName, int Mass, double Abundancy, double Fraction) :
        Name(ElementName), Mass(Mass), Abundancy(Abundancy), Fraction(Fraction), StatWeight(Abundancy*Fraction) {}
    AElasticScatterElement() {}

    bool operator==(const AElasticScatterElement& other) const;

    void writeToJson(QJsonObject& json);
    const QJsonObject writeToJson();
    bool readFromJson(QJsonObject& json);
};

#endif // AELASTICSCATTERELEMENT_H
