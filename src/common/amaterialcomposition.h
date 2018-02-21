#ifndef AMATERIALCOMPOSITION_H
#define AMATERIALCOMPOSITION_H

#include <QString>
#include <QVector>
#include <QSet>
#include <QMap>
#include <QPair>

class QJsonObject;

class AIsotope
{
public:
    QString Symbol;
    int Mass;
    double Abundancy;  //in %

    AIsotope(const QString Symbol, int Mass, double Abundancy) : Symbol(Symbol), Mass(Mass), Abundancy(Abundancy) {}
    AIsotope() : Symbol("Undefined"), Mass(777), Abundancy(0) {}

    void writeToJson(QJsonObject &json) const;
    void readFromJson(const QJsonObject &json);
};

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

    void writeToJson(QJsonObject& json) const;
    void readFromJson(const QJsonObject& json);
};

class AMaterialComposition
{
public:
    AMaterialComposition();

    void configureNaturalAbunances(const QString FileName_NaturalAbundancies);

    QString setCompositionString(const QString composition);  // return error string if invalid composition, else returns ""
    QString getCompositionString() const {return ElementCompositionString;}

    int countElements() const {return ElementComposition.size();}
    int countIsotopes() const;
    AChemicalElement* getElement(int iElement) {return &ElementComposition[iElement];}
    const AChemicalElement* getElement(int iElement) const {return &ElementComposition[iElement];}
    int getNumberInJointIsotopeList(int iElement, int iIsotope);
    double getMeanAtomMass() const {return MeanAtomMass;}

    const QString print() const;

    void clear();

    void writeToJson(QJsonObject& json) const;
    const QJsonObject writeToJson() const;
    void readFromJson(const QJsonObject& json);

    void CalculateMeanAtomMass();

private:
    QString ElementCompositionString;
    QVector<AChemicalElement> ElementComposition;
    double MeanAtomMass; //mean atom mass of the material (in au)

    QSet<QString> AllPossibleElements; //set with all possible element symbols
    QString FileName_NaturalAbundancies;
    QMap<QString, QVector<QPair<int, double> > > NaturalAbundancies; //Key - element name, contains QVector<mass, abund>

    const QString fillIsotopesWithNaturalAbundances(AChemicalElement &element) const; //return error (empty if all fine)
};

#endif // AMATERIALCOMPOSITION_H
