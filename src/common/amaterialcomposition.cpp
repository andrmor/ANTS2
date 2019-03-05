#include "amaterialcomposition.h"
#include "achemicalelement.h"
#include "aglobalsettings.h"
#include "aisotopeabundancehandler.h"
#include "afiletools.h"
#include "ajsontools.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QDebug>
#include <QVector>

#include "TString.h"
#include "TGeoMaterial.h"

QString AMaterialComposition::setCompositionString(const QString composition, bool KeepIsotopComposition)
{
    const AIsotopeAbundanceHandler & IsoAbHandler = AGlobalSettings::getInstance().getIsotopeAbundanceHandler();

    if ( IsoAbHandler.isNaturalAbundanceTableEmpty() )
        return "Configuration error: Table with natural abundancies was not loaded";

    QVector<AChemicalElement> OldElementComposition = ElementComposition;

    QString str = composition.simplified();
    str.replace(" ","+");

    if (str.isEmpty()) return "Material composition is empty!";

    //split in records of molecules (molecule:fraction)
    QStringList elList = str.split(QRegExp("\\+"), QString::SkipEmptyParts );
    QVector< QPair< QString, double> > Records;
    for (QString el : elList)
      {
        QStringList wList = el.split(":");
        if (wList.isEmpty() || wList.size() > 2) return "Format error in composition string!";
        QString Formula = wList.first();
        double Fraction;
        if (wList.size() == 1) Fraction = 1.0;
        else
        {
            QString fr = wList.last();
            bool bOK;
            Fraction = fr.toDouble(&bOK);
            if (!bOK) return "Format error in composition string!";
        }
        Records << QPair<QString,double>(Formula, Fraction);
      }
    //      qDebug() << Records;

    //reading elements and their relative fractions
    QMap<QString,double> map;
    for (QPair<QString,double> pair : Records)
    {
        QString Formula = pair.first.simplified() + ":"; //":" is end signal
        if (!Formula[0].isLetter() && !Formula[0].isUpper()) return "Format error in composition!";
        double weight = pair.second;

        //      qDebug() << Formula;
        bool bReadingElementName = true;
        QString tmp = Formula.at(0);
        QString Element;
        for (int i=1; i<Formula.size(); i++)
        {
            QChar c = Formula[i];
            if (bReadingElementName)
            {
                if (c.isLetter() && c.isLower()) //continue to acquire the name
                {
                    tmp += c;
                    continue;
                }
                else if (c == ':' || c.isDigit() || (c.isLetter() && c.isUpper()))
                {
                    if (tmp.isEmpty()) return "Format error in composition: unrecognized character";
                    Element = tmp;
                    if (c == ':' || (c.isLetter() && c.isUpper()))
                    {
                        if (map.contains(Element)) map[Element] += weight * 1.0;
                        else map[Element] = weight * 1.0;
                        if (c == ':') break;
                    }
                    tmp = QString(c);
                    if (c.isDigit()) bReadingElementName = false;
                }
                else return "Format error in composition: unrecognized character";
            }
            else
            {
                //reading number
                if (c.isDigit())
                {
                    tmp += c;
                    continue;
                }
                else if (c==':' || c.isLetter())
                {
                    if (c.isLower()) return "Format error in composition: lower register symbol in the first character of element symbol";
                    double number = tmp.toDouble();
                    if (map.contains(Element)) map[Element] += weight * number;
                    else map[Element] = weight * number;
                    if (c==':') break;
                    tmp = QString(c);
                    bReadingElementName = true;
                }
                else return "Format error in composition: unrecognized character";
            }
        }

    }
    //      qDebug() << map;

    QList<QString> ListOfElements = map.keys();
    double sumFractions = 0;
    //checking elements
    for (const QString& el : ListOfElements)
    {
        if (!IsoAbHandler.isElementExist(el))
            return QString("Unknown element: %1").arg(el);

        sumFractions += map[el];
    }
    if (sumFractions == 0) return "No elements defined!";

    //filling natural abundancies
    std::sort(ListOfElements.begin(), ListOfElements.end());
    QVector<AChemicalElement> tmpElements;
    for (const QString& ElementName : ListOfElements)
      {
        double MolarFraction = map[ElementName] / sumFractions;
        //qDebug() << ElementName << MolarFraction;

        bool bMakeNew = true;
        AChemicalElement element;

        if (KeepIsotopComposition)
        {
            for (AChemicalElement& el : OldElementComposition)
                if (el.Symbol == ElementName)
                {
                    element = el;
                    element.MolarFraction = MolarFraction;
                    bMakeNew = false;
                    break;
                }
        }

        if ( bMakeNew )
        {
            element = AChemicalElement(ElementName, MolarFraction);
            QString error = IsoAbHandler.fillIsotopesWithNaturalAbundances(element);
            if (!error.isEmpty()) return error;
        }

        tmpElements << element;
      }

    ElementComposition = tmpElements;
    ElementCompositionString = composition.simplified();

    //calculating mean atom mass
    CalculateMeanAtomMass();

    return "";
}

void AMaterialComposition::CalculateMeanAtomMass()
{
    MeanAtomMass = 0;
    for (const AChemicalElement& el : ElementComposition)
        for (const AIsotope& iso : el.Isotopes)
        {
            qDebug() << iso.Symbol << iso.Mass <<" fract:" <<el.MolarFraction <<"aband:"<< iso.Abundancy;
            MeanAtomMass += iso.Mass * el.MolarFraction * 0.01*iso.Abundancy;
        }
    qDebug() << "Mean atom mass is"<< MeanAtomMass;
}

const QString AMaterialComposition::checkForErrors() const
{
    for (const AChemicalElement& el : ElementComposition)
    {
        double sumPercents = 0;
        for (const AIsotope& iso : el.Isotopes)
        {
            //qDebug() << el.Symbol << iso.Mass << iso.Abundancy;
            sumPercents += iso.Abundancy;
        }

        if (sumPercents < 99.9 || sumPercents > 100.1)
            return QString("Isotope fractions for element ") + el.Symbol + " do not add up to 100%\nTolerance is 0.1%, current sum is " + QString::number(sumPercents);
    }

    return "";
}

TGeoMaterial *AMaterialComposition::generateTGeoMaterial(const QString &MatName, const double& density) const
{
    const AIsotopeAbundanceHandler & IsoAbHandler = AGlobalSettings::getInstance().getIsotopeAbundanceHandler();

    TString tName = MatName.toLocal8Bit().data();

    int numElements = countElements();
    int numIsotopes = countIsotopes();

    if (numElements == 0)
        return new TGeoMaterial(tName, 1.0, 1, 1.0e-29);
    else if (numIsotopes == 1)
        return new TGeoMaterial(tName, IsoAbHandler.generateTGeoElement(getElement(0), tName), density);
    else
    {
        TGeoMixture* mix = new TGeoMixture(tName, numElements, density);

        double totWeight = 0;
        for (const AChemicalElement& el : ElementComposition)
            totWeight += el.getFractionWeight() * el.MolarFraction;

        for (const AChemicalElement& el : ElementComposition)
            mix->AddElement(IsoAbHandler.generateTGeoElement(&el, tName), el.getFractionWeight() * el.MolarFraction / totWeight);

        return mix;
    }
}

int AMaterialComposition::countIsotopes() const
{
    int count = 0;
    for (const AChemicalElement& el : ElementComposition)
        count += el.countIsotopes();
    return count;
}

int AMaterialComposition::getNumberInJointIsotopeList(int iElement, int iIsotope)
{
    int iRet = 0;
    for (int iEl=0; iEl<ElementComposition.size(); iEl++)
        for (int iIso=0; iIso<ElementComposition.at(iEl).Isotopes.size(); iIso++)
        {
            if (iEl == iElement && iIso == iIsotope) return iRet;
            iRet++;
        }
    return -1;
}

const QString AMaterialComposition::print() const
{
    QString str = "Composition:" + ElementCompositionString + "\n";
    for (const AChemicalElement& el : ElementComposition)
        str += el.print();
    return str;
}

void AMaterialComposition::clear()
{
    ElementCompositionString = "";
    ElementComposition.clear();
}

void AMaterialComposition::writeToJson(QJsonObject &json) const
{
    json["ElementCompositionString"] = ElementCompositionString;

    QJsonArray ar;
    for (const AChemicalElement& el : ElementComposition)
    {
        QJsonObject js;
        el.writeToJson(js);
        ar << js;
    }
    json["ElementComposition"] = ar;

    json["MeanAtomMass"] = MeanAtomMass;
}

const QJsonObject AMaterialComposition::writeToJson() const
{
    QJsonObject js;
    writeToJson(js);
    return js;
}

void AMaterialComposition::readFromJson(const QJsonObject &json)
{
    parseJson(json, "ElementCompositionString", ElementCompositionString);
    QJsonArray ar;
    parseJson(json, "ElementComposition", ar);

    ElementComposition.clear();
    for (int i=0; i<ar.size(); i++)
    {
        QJsonObject js = ar[i].toObject();
        AChemicalElement el;
        el.readFromJson(js);
        ElementComposition << el;
    }

    parseJson(json, "MeanAtomMass", MeanAtomMass);
}
