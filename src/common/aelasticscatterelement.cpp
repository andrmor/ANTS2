#include "aelasticscatterelement.h"
#include "ajsontools.h"

bool AElasticScatterElement::operator==(const AElasticScatterElement &other) const
{
    if (Name != other.Name) return false;
    if (Mass != other.Mass) return false;
    if (Abundancy != other.Abundancy) return false;
    if (Fraction != other.Fraction) return false;
    if (Energy != other.Energy) return false;
    if (CrossSection != other.CrossSection) return false;
    return true;
}

void AElasticScatterElement::writeToJson(QJsonObject &json)
{
    json["Name"] = Name;
    json["Mass"] = Mass;
    json["Abundancy"] = Abundancy;
    json["Fraction"] = Fraction;

    QJsonArray ar;
    for (int i=0; i<Energy.size(); i++)
    {
        QJsonArray el;
        el << Energy.at(i) << CrossSection.at(i);
        ar << el;
    }
    json["CrossSection"] = ar;

    json["Expanded"] = bExpanded;
}

const QJsonObject AElasticScatterElement::writeToJson()
{
    QJsonObject json;
    writeToJson(json);
    return json;
}

bool AElasticScatterElement::readFromJson(QJsonObject &json)
{
    //if (!isContainAllKeys(json, QStringList()<<"Name"<<"Mass"<<"StatWeight")) return false;

    parseJson(json, "Name", Name);
    parseJson(json, "Mass", Mass);
    Abundancy = 100.0;
    parseJson(json, "Abundancy", Abundancy);
    parseJson(json, "Fraction", Fraction);
    parseJson(json, "StatWeight", Fraction);

    Energy.clear();
    CrossSection.clear();
    if (json.contains("CrossSection"))
    {
        QJsonArray ar = json["CrossSection"].toArray();
        for (int i=0; i<ar.size(); i++)
        {
            QJsonArray el = ar[i].toArray();
            Energy << el[0].toDouble();
            CrossSection << el[1].toDouble();
        }
    }

    parseJson(json, "Expanded", bExpanded);
    return true;
}
