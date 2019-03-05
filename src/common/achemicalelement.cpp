#include "achemicalelement.h"
#include "ajsontools.h"

const QString AChemicalElement::print() const
{
    QString str = "Element: " + Symbol;
    str += "  Relative fraction: " + QString::number(MolarFraction) + "\n";
    str += "Isotopes and their abundances (%):\n";
    for (const AIsotope& iso : Isotopes)
        str += "  " + iso.Symbol + "-" + QString::number(iso.Mass) + "   " + QString::number(iso.Abundancy) + "\n";
    return str;
}

double AChemicalElement::getFractionWeight() const
{
    double w = 0;
    for (const AIsotope& iso : Isotopes)
        w += iso.Mass * iso.Abundancy;
    return w;
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

