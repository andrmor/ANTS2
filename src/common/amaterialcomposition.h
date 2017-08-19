#ifndef AMATERIALCOMPOSITION_H
#define AMATERIALCOMPOSITION_H

#include <QString>
#include <QVector>
#include <QSet>
#include <QMap>
#include <QPair>

class AIsotope
{
public:
    QString Symbol;
    int Mass;
    double Abundancy;

    AIsotope(const QString Symbol, int Mass, double Abundancy) : Symbol(Symbol), Mass(Mass), Abundancy(Abundancy) {}
    AIsotope() : Symbol("Undefined"), Mass(777), Abundancy(0) {}
};

class AChemicalElement
{
public:
    QString Symbol;
    QVector<AIsotope> Isotopes;
    double RelativeFraction;

    AChemicalElement(const QString Symbol, double RelativeFraction) : Symbol(Symbol), RelativeFraction(RelativeFraction) {}
    AChemicalElement() : Symbol("Undefined"), RelativeFraction(0) {}

    const QString print() const;
};

class AMaterialComposition
{
public:
    AMaterialComposition(QString FileName_NaturalAbundancies);

    QString setCompositionString(const QString composition);  // return error string if invalid composition, else returns ""
    QString getCompositionString() const {return ElementCompositionString;}

    const QString print() const;

private:
    QString ElementCompositionString;

    QVector<AChemicalElement> ElementComposition;


    QSet<QString> AllPossibleElements; //set with all possible element symbols
    QString FileName_NaturalAbundancies;
    QMap<QString, QVector<QPair<int, double> > > NaturalAbundancies; //Key - element name, contains QVector<mass, abund>

    const QString fillIsotopesWithNaturalAbundances(AChemicalElement &element) const; //return error (empty if all fine)
};

#endif // AMATERIALCOMPOSITION_H
