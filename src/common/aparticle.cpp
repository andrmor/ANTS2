#include "aparticle.h"

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

void AParticle::readFromJson(QJsonObject &json)
{
  ParticleName = json["Name"].toString();
  type = static_cast<ParticleType>(json["Type"].toInt());
  mass = json["Mass"].toDouble();
  charge = json["Charge"].toInt();
}
