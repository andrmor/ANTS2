#include "amaterialcomposition.h"

#include "afiletools.h"
#include "ajsontools.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QDebug>

AMaterialComposition::AMaterialComposition()
{
    AllPossibleElements<<"H"<<"He"<<"Li"<<"Be"<<"B"<<"C"<<"N"<<"O"<<"F"<<"Ne"<<"Na"<<"Mg"<<"Al"<<"Si"<<"P"<<"S"<<"Cl"<<"Ar"<<"K"<<"Ca"<<"Sc"<<"Ti"<<"V"<<"Cr"<<"Mn"<<"Fe"<<"Co"<<"Ni"<<"Cu"<<"Zn"<<"Ga"<<"Ge"<<"As"<<"Se"<<"Br"<<"Kr"<<"Rb"<<"Sr"<<"Y"<<"Zr"<<"Nb"<<"Mo"<<"Tc"<<"Ru"<<"Rh"<<"Pd"<<"Ag"<<"Cd"<<"In"<<"Sn"<<"Sb"<<"Te"<<"I"<<"Xe"<<"Cs"<<"Ba"<<"La"<<"Ce"<<"Pr"<<"Nd"<<"Pm"<<"Sm"<<"Eu"<<"Gd"<<"Tb"<<"Dy"<<"Ho"<<"Er"<<"Tm"<<"Yb"<<"Lu"<<"Hf"<<"Ta"<<"W"<<"Re"<<"Os"<<"Ir"<<"Pt"<<"Au"<<"Hg"<<"Tl"<<"Pb"<<"Bi"<<"Po"<<"At"<<"Rn"<<"Fr"<<"Ra"<<"Ac"<<"Th"<<"Pa"<<"U"<<"Np"<<"Pu"<<"Am"<<"Cm"<<"Bk"<<"Cf"<<"Es";
}

void AMaterialComposition::configureNaturalAbunances(const QString FileName_NaturalAbundancies)
{
    QString NaturalAbundances;
    bool bOK = LoadTextFromFile(FileName_NaturalAbundancies, NaturalAbundances);
    if (!bOK) qWarning() << "Cannot load file with natural abundances: " + FileName_NaturalAbundancies;
    else
    {
        NaturalAbundancies.clear();
        QStringList SL = NaturalAbundances.split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
        for (QString s : SL)
        {
            QStringList f = s.split(QRegExp("\\s+"), QString::SkipEmptyParts);
            if (f.size() != 2) continue;
            bool bOK = true;
            double Abundancy = (f.last()).toDouble(&bOK);
            if (!bOK) continue;
            QString mass = f.first();
            mass.remove(QRegExp("[a-zA-Z]"));
            int Mass = mass.toInt(&bOK);
            if (!bOK) continue;
            QString Element = f.first();
            Element.remove(QRegExp("[0-9]"));
            //qDebug() << Element << Mass << Abundancy;
            QVector<QPair<int, double> > tmp;
            tmp << QPair<int, double>(Mass, Abundancy);
            if (NaturalAbundancies.contains(Element)) (NaturalAbundancies[Element]) << tmp;
            else NaturalAbundancies[Element] = tmp;
        }
    }
}

QString AMaterialComposition::setCompositionString(const QString composition, bool KeepIsotopComposition)
{
    if (NaturalAbundancies.isEmpty()) return "Configuration error: Table with natural abundancies was not loaded";

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
        if (!AllPossibleElements.contains(el)) return QString("Unknown element: ")+el;
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
            QString error = fillIsotopesWithNaturalAbundances(element);
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

const QString AChemicalElement::print() const
{
    QString str = "Element: " + Symbol;
    str += "  Relative fraction: " + QString::number(MolarFraction) + "\n";
    str += "Isotopes and their abundances (%):\n";
    for (const AIsotope& iso : Isotopes)
        str += "  " + iso.Symbol + "-" + QString::number(iso.Mass) + "   " + QString::number(iso.Abundancy) + "\n";
    return str;
}

void AChemicalElement::writeToJson(QJsonObject &json) const
{
    json["Symbol"] = Symbol;
    json["MolarFraction"] = MolarFraction;

    QJsonArray ar;
    for (const AIsotope& iso : Isotopes)
    {
        QJsonObject js;
        iso.writeToJson(js);
        ar << js;
    }
    json["Isotopes"] = ar;
}

void AChemicalElement::readFromJson(const QJsonObject &json)
{
    parseJson(json, "Symbol", Symbol);
    parseJson(json, "MolarFraction", MolarFraction);
    QJsonArray ar;
    parseJson(json, "Isotopes", ar);

    Isotopes.clear();
    for (int i=0; i<ar.size(); i++)
    {
        QJsonObject js = ar[i].toObject();
        AIsotope iso;
        iso.readFromJson(js);
        Isotopes << iso;
    }
}

const QString AMaterialComposition::fillIsotopesWithNaturalAbundances(AChemicalElement& element) const
{
    QString& name = element.Symbol;
    if (!NaturalAbundancies.contains(name)) return QString("Element ") + name + " not found in dataset of natural abundances";

    element.Isotopes.clear();
    QVector<QPair<int, double> > list = NaturalAbundancies[name];
    for (auto& pair : list)
        element.Isotopes << AIsotope(name, pair.first, pair.second);
    return "";
}

void AIsotope::writeToJson(QJsonObject &json) const
{
    json["Symbol"] = Symbol;
    json["Mass"] = Mass;
    json["Abundancy"] = Abundancy;
}

void AIsotope::readFromJson(const QJsonObject &json)
{
    parseJson(json, "Symbol", Symbol);
    parseJson(json, "Mass", Mass);
    parseJson(json, "Abundancy", Abundancy);
}
