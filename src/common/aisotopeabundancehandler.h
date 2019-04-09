#ifndef AISOTOPEABUNDANCEHANDLER_H
#define AISOTOPEABUNDANCEHANDLER_H

#include <QSet>
#include <QMap>
#include <QString>
#include <QPair>

class AChemicalElement;
class TGeoElement;
class TString;

class AIsotopeAbundanceHandler
{
public:
    AIsotopeAbundanceHandler(const QString & NaturalAbundanceFileName);

    bool isNaturalAbundanceTableEmpty() const {return NaturalAbundancies.isEmpty();}
    bool isElementExist(const QString& elSymbol) const {return AllPossibleElements.contains(elSymbol);}

    const QStringList getListOfElements() const;
    int getZ(const QString & Symbol) const;

    const QString fillIsotopesWithNaturalAbundances(AChemicalElement & element) const;

    TGeoElement* generateTGeoElement(const AChemicalElement *el, const TString &matName) const; //does not own!

private:
    QSet<QString> AllPossibleElements; //set with all possible element symbols until and including Einsteinium Es (99)
    QMap<QString, int> SymbolToNumber;
    QString FileName_NaturalAbundancies;
    QMap<QString, QVector<QPair<int, double> > > NaturalAbundancies; //Key - element name, contains QVector<mass, abund>

    const QString NaturalAbundanceFileName;

    void configureNaturalAbunances();
    bool isNatural(const AChemicalElement *el) const;
};

#endif // AISOTOPEABUNDANCEHANDLER_H
