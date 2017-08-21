#ifndef AELASTICSCATTERELEMENT_H
#define AELASTICSCATTERELEMENT_H

#include <QString>
#include <QVector>

class QJsonObject;

class AElasticScatterElement  //rename to AElasticScatterIsotope ?
{
public:
    QString Name;
    int Mass;
    double Abundancy;   // [0, 100] - abundancy of this isotope in %
    double Fraction;    // Relative quantity of this element in the material - applies to all isotopes of the same element!

    QVector<double> Energy;
    QVector<double> CrossSection;

    //runtime properties
    double MolarFraction_runtime;  // = Abundancy * Fraction / Sum (Abundancy * Fraction) over all elements

    //gui-only
    bool bExpanded;

 AElasticScatterElement(QString ElementName, int Mass, double Abundancy, double Fraction) :
 Name(ElementName), Mass(Mass), Abundancy(Abundancy), Fraction(Fraction), bExpanded(true) {}
    AElasticScatterElement(QString ElementName, int Mass, double MolarFraction) :
        Name(ElementName), Mass(Mass), MolarFraction_runtime(MolarFraction), bExpanded(true) {}
    AElasticScatterElement() : bExpanded(true) {}

    bool operator==(const AElasticScatterElement& other) const;

    void writeToJson(QJsonObject& json);
    const QJsonObject writeToJson();
    bool readFromJson(QJsonObject& json);
};

#endif // AELASTICSCATTERELEMENT_H
