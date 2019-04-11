#include "aparticle.h"
#include "ajsontools.h"

#include <QJsonObject>

AParticle::AParticle(QString Name, AParticle::ParticleType Type, int Z, double A) :
    ParticleName(Name), type(Type), ionZ(Z), ionA(A){}

AParticle::AParticle(QString Name, AParticle::ParticleType Type) :
    ParticleName(Name), type(Type){}

AParticle::AParticle(){}

AParticle::AParticle(const AParticle & other)
{
    ParticleName = other.ParticleName;
    type = other.type;
    ionZ = other.ionZ;
    ionA = other.ionA;
}

bool AParticle::operator==(const AParticle &other) const
{
    if (ParticleName != other.ParticleName) return false;
    if (type         != other.type)         return false;
    if (ionZ         != other.ionZ)         return false;
    if (ionA         != other.ionA)         return false;
    return true;
}

bool AParticle::operator!=(const AParticle &other) const
{
    return !(other == *this);
}

void AParticle::writeToJson(QJsonObject &json) const
{
    json["Name"] = ParticleName;
    json["Type"] = type;
    json["A"] = ionA;
    json["Z"] = ionZ;
}

const QJsonObject AParticle::writeToJson() const
{
    QJsonObject js;
    writeToJson(js);
    return js;
}

void AParticle::readFromJson(const QJsonObject &json)
{
    parseJson(json, "name", ParticleName); //some old formats
    parseJson(json, "Name", ParticleName);

    type = _charged_;
    if (json.contains("type"))
        type = static_cast<ParticleType>(json["type"].toInt());
    if (json.contains("Type"))
        type = static_cast<ParticleType>(json["Type"].toInt());

    ionA = -1;
    parseJson(json, "A", ionA);

    ionZ = -1;
    parseJson(json, "Z", ionZ);
}
