#ifndef PARTICLESOURCESCLASS_H
#define PARTICLESOURCESCLASS_H

#include "aparticlegun.h"

#include <QString>
#include <QVector>
#include "TVector3.h"

class AMaterialParticleCollection;
class QJsonObject;
class TRandom2;
class DetectorClass;
struct AParticleSourceRecord;

class LinkedParticleStructure
{
public:
  int iParticle; //indexed according to GunParticles index
  int LinkedTo;  //index of particle it is linked to

  LinkedParticleStructure() {}
  LinkedParticleStructure(int iparticle, int linkedto = -1) {iParticle = iparticle; LinkedTo = linkedto;}
};

class ParticleSourcesClass : public AParticleGun
{
public:
   ParticleSourcesClass(const DetectorClass* Detector, TRandom2* RandGen);
   ~ParticleSourcesClass();

   virtual void Init() override; // !!! has to be called before the first use of "GenerateEvent"!
   virtual QVector<AGeneratedParticle>* GenerateEvent() override; //see Init!!!

   //for remove particle from configuration
   virtual void RemoveParticle(int particleId) override; //should NOT be used to remove one of particles in use! use onIsPareticleInUse first
   virtual bool IsParticleInUse(int particleId, QString& SourceNames) const override;

  //requests
  int size() {return ParticleSourcesData.size();}
  double getTotalActivity();
  AParticleSourceRecord* getSource(int iSource) {return ParticleSourcesData[iSource];}
  AParticleSourceRecord* getLastSource() {return ParticleSourcesData.last();}

  //Source handling - after handling is finished, requires Init() !!!
  void append(AParticleSourceRecord* gunParticle);
  void forget(AParticleSourceRecord* gunParticle);
  bool replace(int iSource, AParticleSourceRecord* gunParticle);
  void remove(int iSource);
  void clear();

  //check consistency of data
  int CheckSource(int isource); // 0 - no errors
  QString getErrorString(int error);

  // save
  bool writeSourceToJson(int iSource, QJsonObject &json); //only one source
  bool writeToJson(QJsonObject &json); //all config
  // load
  bool readSourceFromJson(int iSource, QJsonObject &json);
  bool readFromJson(QJsonObject &json); // all config
  bool LoadGunEnergySpectrum(int iSource, int iParticle, QString fileName);

  TVector3 GenerateRandomDirection();  
  void checkLimitedToMaterial(AParticleSourceRecord *s);

private:  
  //external resources - pointers
  const DetectorClass* Detector;
  AMaterialParticleCollection* MpCollection;
  TRandom2 *RandGen;

  QVector<AParticleSourceRecord*> ParticleSourcesData;
  QVector<double> TotalParticleWeight;  
  double TotalActivity;
  QVector< QVector< QVector<LinkedParticleStructure> > > LinkedPartiles; //[isource] [iparticle] []  (includes the record of the particle iteslf!!!)
                              //full recipe of emission builder (containes particles linked to particles etc up to the top level individual particle)

  QVector<TVector3> CollimationDirection; //[isource] collimation direction 
  QVector<double> CollimationProbability; //[isource] collimation probability: solid angle inside cone / 4Pi
  //QVector<int> CollimationSpreadProduct; //[isource] vector product of collimation and normal (used to check is the new vector inisde collimation cone)

  //utilities
  void CalculateTotalActivity();
  void GeneratePosition(int isource, double *R) const;
  void AddParticleInCone(int isource, int iparticle, QVector<AGeneratedParticle> *GeneratedParticles) const; //QVector - only pointer is transferred!
};

#endif // PARTICLESOURCESCLASS_H
