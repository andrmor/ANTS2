#ifndef APARTICLE_H
#define APARTICLE_H

#include <QString>

class QJsonObject;

class AParticle //properties of the particles
{
public:
  enum ParticleType { _gamma_ = 0, _charged_ = 1, _neutron_ = 2};

  AParticle(QString Name, ParticleType Type, int Charge, double Mass);
  AParticle();

  QString ParticleName;
  ParticleType type; //gamma, charged, neutron
  double mass;
  int charge;

  void writeToJson(QJsonObject &json) const;
  const QJsonObject writeToJson() const;
  void readFromJson(const QJsonObject &json);
};

#endif // APARTICLE_H
