#ifndef AMATERIALCOMPOSITION_H
#define AMATERIALCOMPOSITION_H

#include "achemicalelement.h"

#include <QString>
#include <QVector>
#include <QSet>
#include <QMap>
#include <QPair>

class QJsonObject;
class TGeoMaterial;
class TGeoElement;
class AIsotopeAbundanceHandler;
class TString;

class AMaterialComposition
{
public:
    QString setCompositionString(const QString composition, bool KeepIsotopComposition = false);  // return error string if invalid composition, else returns ""
    QString getCompositionString() const {return ElementCompositionString;}

    int countElements() const {return ElementComposition.size();}
    int countIsotopes() const;
    AChemicalElement* getElement(int iElement) {return &ElementComposition[iElement];}
    const AChemicalElement* getElement(int iElement) const {return &ElementComposition[iElement];}
    int getNumberInJointIsotopeList(int iElement, int iIsotope);
    double getMeanAtomMass() const {return MeanAtomMass;}

    const QString print() const;

    void clear();
    bool isDefined() const {return !ElementComposition.isEmpty();}

    void writeToJson(QJsonObject& json) const;
    const QJsonObject writeToJson() const;
    void readFromJson(const QJsonObject& json);

    void CalculateMeanAtomMass();

    const QString checkForErrors() const; //returns empty string if OK

    TGeoMaterial* generateTGeoMaterial(const QString& MatName, const double& density) const; //does not own!

private:    
    QString ElementCompositionString;
    QVector<AChemicalElement> ElementComposition;
    double MeanAtomMass; //mean atom mass of the material (in au)

};

#endif // AMATERIALCOMPOSITION_H
