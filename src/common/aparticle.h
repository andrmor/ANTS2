#ifndef APARTICLE_H
#define APARTICLE_H

#include <QString>

class QJsonObject;

class AParticle //properties of the particles
{
public:
  enum ParticleType { _gamma_ = 0, _charged_ = 1, _neutron_ = 2};

  AParticle(QString Name, ParticleType Type, int Z, double A);
  AParticle(QString Name, ParticleType Type);
  AParticle();
  AParticle(const AParticle & other);

  bool operator== (const AParticle & other) const;
  bool operator!= (const AParticle & other) const;

  QString ParticleName = "undefined";
  ParticleType type = _charged_; //gamma, charged (charged and ionZ!=-1 then it is ion), neutron
  int ionZ = -1;
  int ionA = -1;

  bool isIon() const {return ionZ != -1;}

  void writeToJson(QJsonObject &json) const;
  const QJsonObject writeToJson() const;
  void readFromJson(const QJsonObject &json);
};

#endif // APARTICLE_H
