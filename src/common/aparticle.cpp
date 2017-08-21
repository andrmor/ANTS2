#include "aparticle.h"
#include "ajsontools.h"

#include <QJsonObject>

AParticle::AParticle(QString Name, ParticleType Type, int Charge, double Mass)
{
  ParticleName = Name;
  type = Type;
  charge = Charge;
  mass = Mass;
}

AParticle::AParticle()
{
  ParticleName = "undefined";
  type = _charged_;
  charge = 1;
  mass = 666;
}

void AParticle::writeToJson(QJsonObject &json) const
{
  json["Name"] = ParticleName;
  json["Type"] = type;
  json["Mass"] = mass;
  json["Charge"] = charge;
}

const QJsonObject AParticle::writeToJson() const
{
    QJsonObject js;
    writeToJson(js);
    return js;
}

void AParticle::readFromJson(const QJsonObject &json)
{
  parseJson(json, "Name", ParticleName);
  type = static_cast<ParticleType>(json["Type"].toInt());
  parseJson(json, "Mass", mass);
  parseJson(json, "Charge", charge);
}
